typedef struct {
  /* input file description */
  unsigned long num_samples;   /* number of samples. default=2^32-1    */
  int num_channels;            /* input number of channels. default=2  */
  int in_samplerate;           /* input_samp_rate. default=44.1kHz     */
  int out_samplerate;          /* output_samp_rate. (usually determined automatically)   */


  /* general control params */
  int gtkflag;                 /* run frame analyzer?       */
  int bWriteVbrTag;            /* add Xing VBR tag?         */
  int quality;                 /* quality setting 0=best,  9=worst  */
  int silent;                  /* disable some status output */
  int mode;                    /* 0,1,2,3 stereo,jstereo,dual channel,mono */
  int mode_fixed;              /* use specified the mode, do not use lame's opinion of the best mode */
  int force_ms;                /* force M/S mode.  requires mode=1 */
  int brate;                   /* bitrate */

  /* frame params */
  int copyright;               /* mark as copyright. default=0 */
  int original;                /* mark as original. default=1 */
  int error_protection;        /* use 2 bytes per frame for a CRC checksum. default=0 */
  int padding_type;            /* 0=no padding, 1=always pad, 2=adjust padding */
  int extension;               /* the MP3 'private extension' bit.  meaningless */

  /* quantization/noise shaping */
  int disable_reservoir;       /* use bit reservoir? */
  int experimentalX;
  int experimentalY;
  int experimentalZ;

  /* VBR control */
  int VBR;
  int VBR_q;
  int VBR_min_bitrate_kbps;
  int VBR_max_bitrate_kbps;


  /* resampling and filtering */
  int lowpassfreq;             /* freq in Hz. 0=lame choses. -1=no filter */
  int highpassfreq;            /* freq in Hz. 0=lame choses. -1=no filter */
  int lowpasswidth;            /* freq width of filter, in Hz (default=15%) */
  int highpasswidth;           /* freq width of filter, in Hz (default=15%) */


  /* input file reading - not used if calling program does the i/o */
  sound_file_format input_format;
  int swapbytes;               /* force byte swapping   default=0 */
  char *inPath;                /* name of input file */
  char *outPath;               /* name of output file. */
  /* Note: outPath must be set if you want Xing VBR or id3 tags
   * written */


  /* psycho acoustics and other aguments which you should not change 
   * unless you know what you are doing  */
  int ATHonly;                 /* only use ATH */
  int noATH;                   /* disable ATH */
  float cwlimit;               /* predictability limit */
  int allow_diff_short;        /* allow blocktypes to differ between channels ? */
  int no_short_blocks;         /* disable short blocks       */
  int emphasis;                /* obsolete */



  /********************************************************************/
  /* internal variables NOT set by calling program, and should not be */
  /* modified by the calling program                                  */
  /********************************************************************/
  long int frameNum;           /* frame counter */
  long totalframes;            /* frames: 0..totalframes-1 (estimate) */
  int encoder_delay;
  int framesize;
  int version;                 /* 0=MPEG2  1=MPEG1 */
  int padding;                 /* padding for the current frame? */
  int mode_gr;                 /* granules per frame */
  int stereo;                  /* number of channels */
  int VBR_min_bitrate;         /* min bitrate index */
  int VBR_max_bitrate;         /* max bitrate index */
  float resample_ratio;        /* input_samp_rate/output_samp_rate */
  int bitrate_index;
  int samplerate_index;
  int mode_ext;

  /* lowpass and highpass filter control */
  float lowpass1, lowpass2;    /* normalized frequency bounds of passband */
  float highpass1, highpass2;  /* normalized frequency bounds of passband */

  /* polyphase filter (filter_type=0)  */
  int lowpass_band;            /* zero bands >= lowpass_band in the polyphase filterbank */
  int highpass_band;           /* zero bands <= highpass_band */



  int filter_type;             /* 0=polyphase filter, 1= FIR filter 2=MDCT filter(bad) */
  int quantization;            /* 0 = ISO formual,  1=best amplitude */
  int noise_shaping;           /* 0 = none 
				  1 = ISO AAC model
				  2 = allow scalefac_select=1  
			       */

  int noise_shaping_stop;      /* 0 = stop at over=0, all scalefacs amplified or
				  a scalefac has reached max value
				  1 = stop when all scalefacs amplified or        
				  a scalefac has reached max value
				  2 = stop when all scalefacs amplified 
			       */

  int psymodel;                /* 0 = none   1=gpsycho */
  int use_best_huffman;        /* 0 = no.  1=outside loop  2=inside loop(slow) */


} lame_global_flags;

