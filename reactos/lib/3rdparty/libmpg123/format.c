/*
	format:routines to deal with audio (output) format

	copyright 2008-14 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, starting with parts of the old audio.c, with only faintly manage to show now

	A Major change from mpg123 <= 1.18 is that all encodings are only really
	disabled when done so via specific build configuration. Otherwise, the
	missing support of decoders to produce a certain format is augmented by
	postprocessing that converts the samples. This means happily creating
	data with higher resolution from less accurate decoder output.

	The main point is to still offer float encoding when the decoding core uses
	a fixed point representation that has only 16 bit output. Actually, that's
	the only point: A fixed-point build needs to create float from 16 bit, also
	32 or 24 bit from the same source. That's all there is to it: Everything else
	is covered by fallback synth functions. It may be a further step to check if
	there are cases where conversion in postprocessing works well enough to omit
	a certain specialized decoder ... but usually, they are justified by some
	special way to get from float to integer to begin with.

	I won't cover the case of faking double output with float/s16 decoders here.
	Double precision output is a thing for experimental builds anyway. Mostly
	theoretical and without a point.
*/

#include "mpg123lib_intern.h"
#include "debug.h"

/* static int chans[NUM_CHANNELS] = { 1 , 2 }; */
static const long my_rates[MPG123_RATES] = /* only the standard rates */
{
	 8000, 11025, 12000, 
	16000, 22050, 24000,
	32000, 44100, 48000,
};

static const int my_encodings[MPG123_ENCODINGS] =
{
	MPG123_ENC_SIGNED_16,
	MPG123_ENC_UNSIGNED_16,
	MPG123_ENC_SIGNED_32,
	MPG123_ENC_UNSIGNED_32,
	MPG123_ENC_SIGNED_24,
	MPG123_ENC_UNSIGNED_24,
	/* Floating point range, see below. */
	MPG123_ENC_FLOAT_32,
	MPG123_ENC_FLOAT_64,
	/* 8 bit range, see below. */
	MPG123_ENC_SIGNED_8,
	MPG123_ENC_UNSIGNED_8,
	MPG123_ENC_ULAW_8,
	MPG123_ENC_ALAW_8
};

/* Make that match the above table.
   And yes, I still don't like this kludgy stuff. */
/* range[0] <= i < range[1] for forced floating point */
static const int enc_float_range[2] = { 6, 8 };
/* same for 8 bit encodings */
static const int enc_8bit_range[2] = { 8, 12 };

/*
	Only one type of float is supported.
	Actually, double is a very special experimental case not occuring in normal
	builds. Might actually get rid of it.

	Remember here: Also with REAL_IS_FIXED, I want to be able to produce float
	output (f32) via post-processing.
*/
# ifdef REAL_IS_DOUBLE
#  define MPG123_FLOAT_ENC MPG123_ENC_FLOAT_64
# else
#  define MPG123_FLOAT_ENC MPG123_ENC_FLOAT_32
# endif

/* The list of actually possible encodings. */
static const int good_encodings[] =
{
#ifndef NO_16BIT
	MPG123_ENC_SIGNED_16,
	MPG123_ENC_UNSIGNED_16,
#endif
#ifndef NO_32BIT
	MPG123_ENC_SIGNED_32,
	MPG123_ENC_UNSIGNED_32,
	MPG123_ENC_SIGNED_24,
	MPG123_ENC_UNSIGNED_24,
#endif
#ifndef NO_REAL
	MPG123_FLOAT_ENC,
#endif
#ifndef NO_8BIT
	MPG123_ENC_SIGNED_8,
	MPG123_ENC_UNSIGNED_8,
	MPG123_ENC_ULAW_8,
	MPG123_ENC_ALAW_8
#endif
};

/* Check if encoding is a valid one in this build.
   ...lazy programming: linear search. */
static int good_enc(const int enc)
{
	size_t i;
	for(i=0; i<sizeof(good_encodings)/sizeof(int); ++i)
	if(enc == good_encodings[i]) return TRUE;

	return FALSE;
}

void attribute_align_arg mpg123_rates(const long **list, size_t *number)
{
	if(list   != NULL) *list   = my_rates;
	if(number != NULL) *number = sizeof(my_rates)/sizeof(long);
}

/* Now that's a bit tricky... One build of the library knows only a subset of the encodings. */
void attribute_align_arg mpg123_encodings(const int **list, size_t *number)
{
	if(list   != NULL) *list   = good_encodings;
	if(number != NULL) *number = sizeof(good_encodings)/sizeof(int);
}

int attribute_align_arg mpg123_encsize(int encoding)
{
	if(encoding & MPG123_ENC_8)
	return 1;
	else if(encoding & MPG123_ENC_16)
	return 2;
	else if(encoding & MPG123_ENC_24)
	return 3;
	else if(encoding & MPG123_ENC_32 || encoding == MPG123_ENC_FLOAT_32)
	return 4;
	else if(encoding == MPG123_ENC_FLOAT_64)
	return 8;
	else
	return 0;
}

/*	char audio_caps[NUM_CHANNELS][MPG123_RATES+1][MPG123_ENCODINGS]; */

static int rate2num(mpg123_pars *mp, long r)
{
	int i;
	for(i=0;i<MPG123_RATES;i++) if(my_rates[i] == r) return i;
#ifndef NO_NTOM
	if(mp && mp->force_rate != 0 && mp->force_rate == r) return MPG123_RATES;
#endif

	return -1;
}

static int enc2num(int encoding)
{
	int i;
	for(i=0;i<MPG123_ENCODINGS;++i)
	if(my_encodings[i] == encoding) return i;

	return -1;
}

static int cap_fit(mpg123_handle *fr, struct audioformat *nf, int f0, int f2)
{
	int i;
	int c  = nf->channels-1;
	int rn = rate2num(&fr->p, nf->rate);
	if(rn >= 0)	for(i=f0;i<f2;i++)
	{
		if(fr->p.audio_caps[c][rn][i])
		{
			nf->encoding = my_encodings[i];
			return 1;
		}
	}
	return 0;
}

static int freq_fit(mpg123_handle *fr, struct audioformat *nf, int f0, int f2)
{
	nf->rate = frame_freq(fr)>>fr->p.down_sample;
	if(cap_fit(fr,nf,f0,f2)) return 1;
	if(fr->p.flags & MPG123_AUTO_RESAMPLE)
	{
		nf->rate>>=1;
		if(cap_fit(fr,nf,f0,f2)) return 1;
		nf->rate>>=1;
		if(cap_fit(fr,nf,f0,f2)) return 1;
	}
#ifndef NO_NTOM
	/* If nothing worked, try the other rates, only without constrains from user.
	   In case you didn't guess: We enable flexible resampling if we find a working rate. */
	if(  fr->p.flags & MPG123_AUTO_RESAMPLE &&
	    !fr->p.force_rate && fr->p.down_sample == 0)
	{
		int i;
		int c  = nf->channels-1;
		int rn = rate2num(&fr->p, frame_freq(fr));
		int rrn;
		if(rn < 0) return 0;
		/* Try higher rates first. */
		for(i=f0;i<f2;i++) for(rrn=rn+1; rrn<MPG123_RATES; ++rrn)
		if(fr->p.audio_caps[c][rrn][i])
		{
			nf->rate = my_rates[rrn];
			nf->encoding = my_encodings[i];
			return 1;
		}
		/* Then lower rates. */
		for(i=f0;i<f2;i++) for(rrn=rn-1; rrn>=0; --rrn)
		if(fr->p.audio_caps[c][rrn][i])
		{
			nf->rate = my_rates[rrn];
			nf->encoding = my_encodings[i];
			return 1;
		}
	}
#endif

	return 0;
}

/* match constraints against supported audio formats, store possible setup in frame
  return: -1: error; 0: no format change; 1: format change */
int frame_output_format(mpg123_handle *fr)
{
	struct audioformat nf;
	int f0=0;
	int f2=MPG123_ENCODINGS; /* Omit the 32bit and float encodings. */
	mpg123_pars *p = &fr->p;
	/* initialize new format, encoding comes later */
	nf.channels = fr->stereo;

	/* All this forcing should be removed in favour of the capabilities table... */
	if(p->flags & MPG123_FORCE_8BIT)
	{
		f0 = enc_8bit_range[0];
		f2 = enc_8bit_range[1];
	}
	if(p->flags & MPG123_FORCE_FLOAT)
	{
		f0 = enc_float_range[0];
		f2 = enc_float_range[1];
	}

	/* force stereo is stronger */
	if(p->flags & MPG123_FORCE_MONO)   nf.channels = 1;
	if(p->flags & MPG123_FORCE_STEREO) nf.channels = 2;

#ifndef NO_NTOM
	if(p->force_rate)
	{
		nf.rate = p->force_rate;
		if(cap_fit(fr,&nf,f0,2)) goto end;            /* 16bit encodings */
		if(cap_fit(fr,&nf,f0<=2 ? 2 : f0,f2)) goto end; /*  8bit encodings */

		/* try again with different stereoness */
		if(nf.channels == 2 && !(p->flags & MPG123_FORCE_STEREO)) nf.channels = 1;
		else if(nf.channels == 1 && !(p->flags & MPG123_FORCE_MONO)) nf.channels = 2;

		if(cap_fit(fr,&nf,f0,2)) goto end;            /* 16bit encodings */
		if(cap_fit(fr,&nf,f0<=2 ? 2 : f0,f2)) goto end; /*  8bit encodings */

		if(NOQUIET)
		error3( "Unable to set up output format! Constraints: %s%s%liHz.",
		        ( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
		          (p->flags & MPG123_FORCE_MONO ? "mono, " : "") ),
		        (p->flags & MPG123_FORCE_8BIT ? "8bit, " : ""),
		        p->force_rate );
/*		if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

		fr->err = MPG123_BAD_OUTFORMAT;
		return -1;
	}
#endif

	if(freq_fit(fr, &nf, f0, 2)) goto end; /* try rates with 16bit */
	if(freq_fit(fr, &nf, f0<=2 ? 2 : f0, f2)) goto end; /* ... 8bit */

	/* try again with different stereoness */
	if(nf.channels == 2 && !(p->flags & MPG123_FORCE_STEREO)) nf.channels = 1;
	else if(nf.channels == 1 && !(p->flags & MPG123_FORCE_MONO)) nf.channels = 2;

	if(freq_fit(fr, &nf, f0, 2)) goto end; /* try rates with 16bit */
	if(freq_fit(fr, &nf,  f0<=2 ? 2 : f0, f2)) goto end; /* ... 8bit */

	/* Here is the _bad_ end. */
	if(NOQUIET)
	{
		error5( "Unable to set up output format! Constraints: %s%s%li, %li or %liHz.",
		        ( p->flags & MPG123_FORCE_STEREO ? "stereo, " :
		          (p->flags & MPG123_FORCE_MONO ? "mono, "  : "") ),
		        (p->flags & MPG123_FORCE_8BIT  ? "8bit, " : ""),
		        frame_freq(fr),  frame_freq(fr)>>1, frame_freq(fr)>>2 );
	}
/*	if(NOQUIET && p->verbose <= 1) print_capabilities(fr); */

	fr->err = MPG123_BAD_OUTFORMAT;
	return -1;

end: /* Here is the _good_ end. */
	/* we had a successful match, now see if there's a change */
	if(nf.rate == fr->af.rate && nf.channels == fr->af.channels && nf.encoding == fr->af.encoding)
	{
		debug2("Old format with %i channels, and FORCE_MONO=%li", nf.channels, p->flags & MPG123_FORCE_MONO);
		return 0; /* the same format as before */
	}
	else /* a new format */
	{
		debug1("New format with %i channels!", nf.channels);
		fr->af.rate = nf.rate;
		fr->af.channels = nf.channels;
		fr->af.encoding = nf.encoding;
		/* Cache the size of one sample in bytes, for ease of use. */
		fr->af.encsize = mpg123_encsize(fr->af.encoding);
		if(fr->af.encsize < 1)
		{
			if(NOQUIET) error1("Some unknown encoding??? (%i)", fr->af.encoding);

			fr->err = MPG123_BAD_OUTFORMAT;
			return -1;
		}
		/* Set up the decoder synth format. Might differ. */
#ifdef NO_SYNTH32
		/* Without high-precision synths, 16 bit signed is the basis for
		   everything higher than 8 bit. */
		if(fr->af.encsize > 2)
		fr->af.dec_enc = MPG123_ENC_SIGNED_16;
		else
		{
#endif
			switch(fr->af.encoding)
			{
#ifndef NO_32BIT
			case MPG123_ENC_SIGNED_24:
			case MPG123_ENC_UNSIGNED_24:
			case MPG123_ENC_UNSIGNED_32:
				fr->af.dec_enc = MPG123_ENC_SIGNED_32;
			break;
#endif
#ifndef NO_16BIT
			case MPG123_ENC_UNSIGNED_16:
				fr->af.dec_enc = MPG123_ENC_SIGNED_16;
			break;
#endif
			default:
				fr->af.dec_enc = fr->af.encoding;
			}
#ifdef NO_SYNTH32
		}
#endif
		fr->af.dec_encsize = mpg123_encsize(fr->af.dec_enc);
		return 1;
	}
}

int attribute_align_arg mpg123_format_none(mpg123_handle *mh)
{
	int r;
	if(mh == NULL) return MPG123_BAD_HANDLE;

	r = mpg123_fmt_none(&mh->p);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }

	return r;
}

int attribute_align_arg mpg123_fmt_none(mpg123_pars *mp)
{
	if(mp == NULL) return MPG123_BAD_PARS;

	if(PVERB(mp,3)) fprintf(stderr, "Note: Disabling all formats.\n");

	memset(mp->audio_caps,0,sizeof(mp->audio_caps));
	return MPG123_OK;
}

int attribute_align_arg mpg123_format_all(mpg123_handle *mh)
{
	int r;
	if(mh == NULL) return MPG123_BAD_HANDLE;

	r = mpg123_fmt_all(&mh->p);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }

	return r;
}

int attribute_align_arg mpg123_fmt_all(mpg123_pars *mp)
{
	size_t rate, ch, enc;
	if(mp == NULL) return MPG123_BAD_PARS;

	if(PVERB(mp,3)) fprintf(stderr, "Note: Enabling all formats.\n");

	for(ch=0;   ch   < NUM_CHANNELS;     ++ch)
	for(rate=0; rate < MPG123_RATES+1;   ++rate)
	for(enc=0;  enc  < MPG123_ENCODINGS; ++enc)
	mp->audio_caps[ch][rate][enc] = good_enc(my_encodings[enc]) ? 1 : 0;

	return MPG123_OK;
}

int attribute_align_arg mpg123_format(mpg123_handle *mh, long rate, int channels, int encodings)
{
	int r;
	if(mh == NULL) return MPG123_BAD_HANDLE;
	r = mpg123_fmt(&mh->p, rate, channels, encodings);
	if(r != MPG123_OK){ mh->err = r; r = MPG123_ERR; }

	return r;
}

int attribute_align_arg mpg123_fmt(mpg123_pars *mp, long rate, int channels, int encodings)
{
	int ie, ic, ratei;
	int ch[2] = {0, 1};
	if(mp == NULL) return MPG123_BAD_PARS;
	if(!(channels & (MPG123_MONO|MPG123_STEREO))) return MPG123_BAD_CHANNEL;

	if(PVERB(mp,3)) fprintf(stderr, "Note: Want to enable format %li/%i for encodings 0x%x.\n", rate, channels, encodings);

	if(!(channels & MPG123_STEREO)) ch[1] = 0;     /* {0,0} */
	else if(!(channels & MPG123_MONO)) ch[0] = 1; /* {1,1} */
	ratei = rate2num(mp, rate);
	if(ratei < 0) return MPG123_BAD_RATE;

	/* now match the encodings */
	for(ic = 0; ic < 2; ++ic)
	{
		for(ie = 0; ie < MPG123_ENCODINGS; ++ie)
		if(good_enc(my_encodings[ie]) && ((my_encodings[ie] & encodings) == my_encodings[ie]))
		mp->audio_caps[ch[ic]][ratei][ie] = 1;

		if(ch[0] == ch[1]) break; /* no need to do it again */
	}

	return MPG123_OK;
}

int attribute_align_arg mpg123_format_support(mpg123_handle *mh, long rate, int encoding)
{
	if(mh == NULL) return 0;
	else return mpg123_fmt_support(&mh->p, rate, encoding);
}

int attribute_align_arg mpg123_fmt_support(mpg123_pars *mp, long rate, int encoding)
{
	int ch = 0;
	int ratei, enci;
	ratei = rate2num(mp, rate);
	enci  = enc2num(encoding);
	if(mp == NULL || ratei < 0 || enci < 0) return 0;
	if(mp->audio_caps[0][ratei][enci]) ch |= MPG123_MONO;
	if(mp->audio_caps[1][ratei][enci]) ch |= MPG123_STEREO;
	return ch;
}

/* Call this one to ensure that any valid format will be something different than this. */
void invalidate_format(struct audioformat *af)
{
	af->encoding = 0;
	af->rate     = 0;
	af->channels = 0;
}

/* Number of bytes the decoder produces. */
off_t decoder_synth_bytes(mpg123_handle *fr, off_t s)
{
	return s * fr->af.dec_encsize * fr->af.channels;
}

/* Samples/bytes for output buffer after post-processing. */
/* take into account: channels, bytes per sample -- NOT resampling!*/
off_t samples_to_bytes(mpg123_handle *fr , off_t s)
{
	return s * fr->af.encsize * fr->af.channels;
}

off_t bytes_to_samples(mpg123_handle *fr , off_t b)
{
	return b / fr->af.encsize / fr->af.channels;
}

/* Number of bytes needed for decoding _and_ post-processing. */
off_t outblock_bytes(mpg123_handle *fr, off_t s)
{
	int encsize = (fr->af.encoding & MPG123_ENC_24)
	? 4 /* Intermediate 32 bit. */
	: (fr->af.encsize > fr->af.dec_encsize
		? fr->af.encsize
		: fr->af.dec_encsize);
	return s * encsize * fr->af.channels;
}

#ifndef NO_32BIT
/* Remove every fourth byte, facilitating conversion from 32 bit to 24 bit integers.
   This has to be aware of endianness, of course. */
static void chop_fourth_byte(struct outbuffer *buf)
{
	unsigned char *wpos = buf->data;
	unsigned char *rpos = buf->data;
#ifdef WORDS_BIGENDIAN
	while((size_t) (rpos - buf->data + 4) <= buf->fill)
	{
		/* Really stupid: Copy, increment. Byte per byte. */
		*wpos = *rpos;
		wpos++; rpos++;
		*wpos = *rpos;
		wpos++; rpos++;
		*wpos = *rpos;
		wpos++; rpos++;
		rpos++; /* Skip the lowest byte (last). */
	}
#else
	while((size_t) (rpos - buf->data + 4) <= buf->fill)
	{
		/* Really stupid: Copy, increment. Byte per byte. */
		rpos++; /* Skip the lowest byte (first). */
		*wpos = *rpos;
		wpos++; rpos++;
		*wpos = *rpos;
		wpos++; rpos++;
		*wpos = *rpos;
		wpos++; rpos++;
	}
#endif
	buf->fill = wpos-buf->data;
}

static void conv_s32_to_u32(struct outbuffer *buf)
{
	size_t i;
	int32_t  *ssamples = (int32_t*)  buf->data;
	uint32_t *usamples = (uint32_t*) buf->data;
	size_t count = buf->fill/sizeof(int32_t);

	for(i=0; i<count; ++i)
	{
		/* Different strategy since we don't have a larger type at hand.
			 Also watch out for silly +-1 fun because integer constants are signed in C90! */
		if(ssamples[i] >= 0)
		usamples[i] = (uint32_t)ssamples[i] + 2147483647+1;
		/* The smallest value goes zero. */
		else if(ssamples[i] == ((int32_t)-2147483647-1))
		usamples[i] = 0;
		/* Now -value is in the positive range of signed int ... so it's a possible value at all. */
		else
		usamples[i] = (uint32_t)2147483647+1 - (uint32_t)(-ssamples[i]);
	}
}

#endif


/* We always assume that whole numbers are written!
   partials will be cut out. */

static const char *bufsizeerr = "Fatal: Buffer too small for postprocessing!";


#ifndef NO_16BIT

static void conv_s16_to_u16(struct outbuffer *buf)
{
	size_t i;
	int16_t  *ssamples = (int16_t*) buf->data;
	uint16_t *usamples = (uint16_t*)buf->data;
	size_t count = buf->fill/sizeof(int16_t);

	for(i=0; i<count; ++i)
	{
		long tmp = (long)ssamples[i]+32768;
		usamples[i] = (uint16_t)tmp;
	}
}

#ifndef NO_REAL
static void conv_s16_to_f32(struct outbuffer *buf)
{
	ssize_t i;
	int16_t *in = (int16_t*) buf->data;
	float  *out = (float*)   buf->data;
	size_t count = buf->fill/sizeof(int16_t);
	/* Does that make any sense? In x86, there is an actual instruction to divide
	   float by integer ... but then, if we have that FPU, we don't really need
	   fixed point decoder hacks ...? */
	float scale = 1./SHORT_SCALE;

	if(buf->size < count*sizeof(float))
	{
		error1("%s", bufsizeerr);
		return;
	}

	/* Work from the back since output is bigger. */
	for(i=count-1; i>=0; --i)
	out[i] = (float)in[i] * scale;

	buf->fill = count*sizeof(float);
}
#endif

#ifndef NO_32BIT
static void conv_s16_to_s32(struct outbuffer *buf)
{
	ssize_t i;
	int16_t  *in = (int16_t*) buf->data;
	int32_t *out = (int32_t*) buf->data;
	size_t count = buf->fill/sizeof(int16_t);

	if(buf->size < count*sizeof(int32_t))
	{
		error1("%s", bufsizeerr);
		return;
	}

	/* Work from the back since output is bigger. */
	for(i=count-1; i>=0; --i)
	{
		out[i] = in[i];
		/* Could just shift bytes, but would have to mess with sign bit. */
		out[i] *= S32_RESCALE;
	}

	buf->fill = count*sizeof(int32_t);
}
#endif
#endif


void postprocess_buffer(mpg123_handle *fr)
{
	/*
		This caters for the final output formats that are never produced by
		decoder synth directly (wide unsigned and 24 bit formats) or that are
		missing because of limited decoder precision (16 bit synth but 32 or
		24 bit output).
	*/
	switch(fr->af.dec_enc)
	{
#ifndef NO_32BIT
	case MPG123_ENC_SIGNED_32:
		switch(fr->af.encoding)
		{
		case MPG123_ENC_UNSIGNED_32:
			conv_s32_to_u32(&fr->buffer);
		break;
		case MPG123_ENC_UNSIGNED_24:
			conv_s32_to_u32(&fr->buffer);
			chop_fourth_byte(&fr->buffer);
		break;
		case MPG123_ENC_SIGNED_24:
			chop_fourth_byte(&fr->buffer);
		break;
		}
	break;
#endif
#ifndef NO_16BIT
	case MPG123_ENC_SIGNED_16:
		switch(fr->af.encoding)
		{
		case MPG123_ENC_UNSIGNED_16:
			conv_s16_to_u16(&fr->buffer);
		break;
#ifndef NO_REAL
		case MPG123_ENC_FLOAT_32:
			conv_s16_to_f32(&fr->buffer);
		break;
#endif
#ifndef NO_32BIT
		case MPG123_ENC_SIGNED_32:
			conv_s16_to_s32(&fr->buffer);
		break;
		case MPG123_ENC_UNSIGNED_32:
			conv_s16_to_s32(&fr->buffer);
			conv_s32_to_u32(&fr->buffer);
		break;
		case MPG123_ENC_UNSIGNED_24:
			conv_s16_to_s32(&fr->buffer);
			conv_s32_to_u32(&fr->buffer);
			chop_fourth_byte(&fr->buffer);
		break;
		case MPG123_ENC_SIGNED_24:
			conv_s16_to_s32(&fr->buffer);
			chop_fourth_byte(&fr->buffer);
		break;
#endif
		}
	break;
#endif
	}
}
