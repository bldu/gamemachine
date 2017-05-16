﻿#include "stdafx.h"
#include "soundreader_mp3.h"
#include "gmdatacore/gamepackage.h"
#include "mad.h"
#ifdef _WINDOWS
#include <dsound.h>
#endif
#include "utilities/comptr.h"
#include "os/directsound_sounddevice.h"

#define FRAME_BUFFER 50 // 每一份缓存包含的帧数
#define BUFFER_COUNT 3 // 将缓存分为多少部分

class MP3SoundFile;
struct MP3SoundFilePrivate
{
	MP3SoundFile* parent;
	bool inited; //是否已经初始化
	bool formatCreated; // 是否已经生成了WAVEFORMATEX
	bool bufferLoaded; // 是否已经读取了所有MP3文件数据
	WAVEFORMATEX format;
	GamePackageBuffer bufferIn; // MP3文件数据
	std::vector<GMbyte> bufferOut; // 缓存的字节
	GMuint bufferOffset; // 缓存偏移
	GMuint bufferSize; // 单位缓存，每次读取这么多大小的缓存
	bool playing; // 是否正在播放
	GMlong frame; // MP3已经解码的帧数

#ifdef _WINDOWS
	ComPtr<IDirectSoundNotify> cpDirectSoundNotify;
	DSBPOSITIONNOTIFY notifyPos[BUFFER_COUNT];
	HANDLE events[BUFFER_COUNT];
	ComPtr<IDirectSoundBuffer8> cpDirectSoundBuffer;
	HANDLE thread;
#endif
};

#ifdef _WINDOWS
static DWORD WINAPI decode(LPVOID lpThreadParameter);
static DWORD WINAPI processBuffer(LPVOID lpThreadParameter);

class MP3SoundFile : public SoundFile
{
	DEFINE_PRIVATE(MP3SoundFile)

	typedef SoundFile Base;

public:
	MP3SoundFile(GamePackageBuffer* in)
		: Base(WAVEFORMATEX(), nullptr)
	{
		D(d);
		d.parent = this;
		d.inited = false;
		d.bufferIn = *in;
		d.playing = false;
		d.formatCreated = false;
		d.bufferLoaded = false;
		d.frame = 0;
		d.bufferOffset = 0;
	}

	~MP3SoundFile()
	{
		D(d);
		for (GMint i = 0; i < BUFFER_COUNT; i++)
		{
			::CloseHandle(d.notifyPos[i].hEventNotify);
		}
	}

public:
	virtual void play() override
	{
		D(d);
		d.playing = true;
		d.thread = ::CreateThread(NULL, NULL, ::decode, &d, NULL, NULL);
	}

	virtual void stop() override
	{
		D(d);
		d.playing = false;
	}

public:
	// 将缓冲填满
	void transfer(GMbyte* b, GMuint size)
	{
		D(d);
		if (!d.inited)
		{
			init(b, size);
			d.inited = true;
			::CreateThread(NULL, NULL, processBuffer, &d, NULL, NULL);
		}
	}

private:
	void init(GMbyte* b, GMuint size)
	{
		D(d);
		DSBUFFERDESC dsbd = { 0 };
		dsbd.dwSize = sizeof(DSBUFFERDESC);
		dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLFX | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = size * BUFFER_COUNT; // 我们先填充一部分数据，剩下的在接到通知后再填充
		dsbd.lpwfxFormat = (LPWAVEFORMATEX)&d.format;
		d.bufferSize = size;

		ComPtr<IDirectSoundBuffer> cpBuffer;
		HRESULT hr;
		if (FAILED(hr = SoundPlayerDevice::getInstance()->CreateSoundBuffer(&dsbd, &cpBuffer, NULL)))
		{
			gm_error("create sound buffer error.");
			return;
		}

		if (FAILED(hr = cpBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&d.cpDirectSoundBuffer)))
		{
			gm_error("QueryInterface to IDirectSoundBuffer8 error");
			return;
		}

		if (FAILED(hr = d.cpDirectSoundBuffer->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&d.cpDirectSoundNotify)))
		{
			gm_error("QueryInterface to IDirectSoundNotify error");
			return;
		}

		for (GMint i = 0; i < BUFFER_COUNT; i++)
		{
			d.notifyPos[i].dwOffset = i * d.bufferSize;
			d.notifyPos[i].hEventNotify = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			d.events[i] = d.notifyPos[i].hEventNotify;
		}
		hr = d.cpDirectSoundNotify->SetNotificationPositions(2, d.notifyPos);
		ASSERT(SUCCEEDED(hr));

		LPVOID lpLockBuf;
		DWORD len;
		hr = d.cpDirectSoundBuffer->Lock(0, 0, &lpLockBuf, &len, 0, 0, DSBLOCK_ENTIREBUFFER);
		ASSERT(SUCCEEDED(hr));
		memcpy(lpLockBuf, b, size);
		memset((GMbyte*)lpLockBuf + size, 0, len - size);
		hr = d.cpDirectSoundBuffer->Unlock(lpLockBuf, len, NULL, NULL);
		ASSERT(SUCCEEDED(hr));
		hr = d.cpDirectSoundBuffer->SetCurrentPosition(0);
		ASSERT(SUCCEEDED(hr));
		hr = d.cpDirectSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
		ASSERT(SUCCEEDED(hr));

	}
};
#endif

static inline GMint scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static mad_flow input(void *data, mad_stream *stream)
{
	MP3SoundFile::Data* d = (MP3SoundFile::Data*)data;

	if (d->bufferLoaded)
		return MAD_FLOW_STOP;

	mad_stream_buffer(stream, d->bufferIn.buffer, d->bufferIn.size);
	d->bufferLoaded = true;
	return MAD_FLOW_CONTINUE;
}

static mad_flow output(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
	MP3SoundFile::Data* d = (MP3SoundFile::Data*)data;

	if (!d->playing)
		return MAD_FLOW_STOP;

	if (!d->formatCreated)
	{
		d->format.cbSize = sizeof(WAVEFORMATEX);
		d->format.nChannels = pcm->channels;
		d->format.nBlockAlign = 4;
		d->format.wFormatTag = 1;
		d->format.nSamplesPerSec = pcm->samplerate;
		d->format.nAvgBytesPerSec = d->format.nSamplesPerSec * sizeof(unsigned short)* pcm->channels;
		d->format.wBitsPerSample = 16;
		d->formatCreated = true;
	}

	GMuint nchannels, nsamples;
	mad_fixed_t const *left_ch, *right_ch;

	/* pcm->samplerate contains the sampling frequency */

	nchannels = pcm->channels;
	nsamples = pcm->length;
	left_ch = pcm->samples[0];
	right_ch = pcm->samples[1];

	while (nsamples--) {
		signed int sample;

		/* output sample(s) in 16-bit signed little-endian PCM */

		sample = scale(*left_ch++);

		d->bufferOut.push_back((sample >> 0) & 0xff);
		d->bufferOut.push_back((sample >> 8) & 0xff);

		if (nchannels == 2) {
			sample = scale(*right_ch++);
			d->bufferOut.push_back((sample >> 0) & 0xff);
			d->bufferOut.push_back((sample >> 8) & 0xff);
		}
	}

	if (d->frame == FRAME_BUFFER * BUFFER_COUNT)
	{
		// 先读取BUFFER_COUNT倍的数据到内存
		GMint transfered = d->bufferOut.size() / BUFFER_COUNT;
		d->parent->transfer(d->bufferOut.data() + d->bufferOffset, transfered);
		d->bufferOffset += transfered;
	}
	if (d->frame < (FRAME_BUFFER * BUFFER_COUNT) + 1)
		d->frame++;

	return MAD_FLOW_CONTINUE;
}

static DWORD WINAPI decode(LPVOID lpThreadParameter)
{
	MP3SoundFile::Data* d = (MP3SoundFile::Data*)lpThreadParameter;
	mad_decoder decoder;
	mad_decoder_init(&decoder, d, input, nullptr, nullptr, output, nullptr, nullptr);
	GMint result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
	return mad_decoder_finish(&decoder) == 0;
}

static DWORD WINAPI processBuffer(LPVOID lpThreadParameter)
{
	MP3SoundFile::Data* d = (MP3SoundFile::Data*)lpThreadParameter;
	GMint part = 0;
	while (d->playing)
	{
		WaitForMultipleObjects(BUFFER_COUNT, d->events, FALSE, INFINITE);
		part = (part + 1) % BUFFER_COUNT;
		HRESULT hr;
		LPVOID lpLockBuf;
		DWORD len;
		hr = d->cpDirectSoundBuffer->Lock(part * d->bufferSize, d->bufferSize, &lpLockBuf, &len, 0, 0, DSBLOCK_ENTIREBUFFER);
		ASSERT(SUCCEEDED(hr));
		memcpy(lpLockBuf, d->bufferOut.data() + d->bufferOffset, d->bufferSize);
		hr = d->cpDirectSoundBuffer->Unlock(lpLockBuf, len, NULL, NULL);
		ASSERT(SUCCEEDED(hr));
		d->bufferOffset += d->bufferSize;
		if (d->bufferOffset > d->bufferOut.size())
		{
			//TODO: do not loop
			break;
		}
	}
	return 0;
}

bool SoundReader_MP3::load(GamePackageBuffer& buffer, OUT ISoundFile** sf)
{
	D(d);
	MP3SoundFile* file = new MP3SoundFile(&buffer);
	*sf = file;
	return 0;
}

bool SoundReader_MP3::test(const GamePackageBuffer& buffer)
{
	return buffer.buffer[0] == 'I' && buffer.buffer[1] == 'D' && buffer.buffer[2] == '3';
}