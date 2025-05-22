/* Copyright (c) 2007-2008 CSIRO
   Copyright (c) 2007-2009 Xiph.Org Foundation
   Written by Jean-Marc Valin */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../include/tlka_opus_api.h"
#include "wav.h"
#include "../src/dump.h"


//  #define ENCODE_ONLY
#define DECODE_ONLY
#define HFP_PLC_DBG

//#define OPUS_CODESIZE

/* in fact, the MAX_PACKET is 1275 */
#define MAX_PACKET       (1500)
/* SILK MAX_FRAME_SIZE is 120ms,48Khz -> 5760 */
#define MAX_FRAME_SIZE   (160*2*5)
#define MAX_CH           (2)

//#define FRAME_MS	50			/* CELT: 1/0.0025 1/0.005 1/0.01 1/0.02 */

#ifdef ANDES_PROFILE
#include <nds_intrinsic.h>
int start_cyc;
int end_cyc;
int max_cyc=0,max_cyc1=0;
int totalcyc =0;
int totalplccyc =0, max_plccyc = 0;
#endif

int frame_count = 0;

void print_usage( char* argv[] )
{
    fprintf(stderr, "Usage: %s [-e] <application> <sampling rate (Hz)> <channels (1/2)> "
        "<bits per second>  [options] <input> <output>\n", argv[0]);
    fprintf(stderr, "       %s -d <sampling rate (Hz)> <channels (1/2)> "
        "[options] <input> <output>\n\n", argv[0]);
    fprintf(stderr, "application: voip | audio | restricted-lowdelay\n" );
    fprintf(stderr, "options:\n" );
    fprintf(stderr, "-e                   : only runs the encoder (output the bit-stream)\n" );
    fprintf(stderr, "-d                   : only runs the decoder (reads the bit-stream as input)\n" );
    fprintf(stderr, "-cbr                 : enable constant bitrate; default: variable bitrate\n" );
    fprintf(stderr, "-cvbr                : enable constrained variable bitrate; default: unconstrained\n" );
    fprintf(stderr, "-delayed-decision    : use look-ahead for speech/music detection (experts only); default: disabled\n" );
    fprintf(stderr, "-bandwidth <NB|MB|WB|SWB|FB> : audio bandwidth (from narrowband to fullband); default: sampling rate\n" );
    fprintf(stderr, "-framesize <2.5|5|10|20|40|60|80|100|120> : frame size in ms; default: 20 \n" );
    fprintf(stderr, "-max_payload <bytes> : maximum payload size in bytes, default: 1024\n" );
    fprintf(stderr, "-complexity <comp>   : complexity, 0 (lowest) ... 10 (highest); default: 10\n" );
    fprintf(stderr, "-inbandfec           : enable SILK inband FEC\n" );
    fprintf(stderr, "-forcemono           : force mono encoding, even for stereo input\n" );
    fprintf(stderr, "-dtx                 : enable SILK DTX\n" );
    fprintf(stderr, "-loss <perc>         : simulate packet loss, in percent (0-100); default: 0\n" );
}

static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
         | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

#ifdef ANDES_PROFILE
void write_stack(int size);
int cal_used_stack(int size);
#endif

#ifdef DECODE_ONLY
void decode_main(int argc, char* argv[])
{
    int frame_size;
    int packet_loss_perc;
    char* inFile, * outFile;
    FILE* fin = NULL;
    FILE* fout = NULL;
    OpusDecoder* dec = NULL;
    int decSize;
    int err;
    short out[MAX_FRAME_SIZE*MAX_CH];
    unsigned char data[MAX_PACKET];
    int stop = 0;
    size_t num_read;
    int len;
    opus_uint32 enc_final_range;
    int lost = 0, lost_prev = 0;
    opus_int32 count = 0, plc_count = 0;
    double tot_samples = 0;
    opus_int32 output_samples;
    unsigned char ch[4];
    WAV_HEADER header;
    int channels;
    opus_int32 sample_rate;
    int frame_ms;
    int max_frame_size = 48000*2;


    void* scratch = NULL;
    int scratchSize;

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
    write_stack(10 * 1024);
#endif

#if __riscv
    inFile = "D:\\Desktop\\test\\Opus\\2_c3.opus";
    outFile = "D:\\Desktop\\test\\Opus\\2_c3_aout.wav";
    frame_ms = 50;

    channels = 1;		 // 1, 2
    sample_rate = 16000; //8k 12k 16k 24k 48k
#else
    inFile = argv[1];
    outFile = argv[2];
    frame_ms = atoi(argv[3]);

    channels =  atoi(argv[4]);		 // 1, 2
    sample_rate =  atoi(argv[5]); //8k 12k 16k 24k 48k
#endif

    /* open input file */
    fin = fopen(inFile, "rb");
    if (!fin)
    {
        printf("can't open input file.\n");
        return;
    }
    /* open output file */
    fout = fopen(outFile, "wb");
    if (!fout)
    {
        printf("can't open output file.\n");
        return;
    }

#ifdef HFP_PLC_DBG
    /* open plc file */
    FILE* fplc = NULL;
    char *plcFile;
    short plcout[MAX_FRAME_SIZE*MAX_CH];

    plcFile = "D:\\Desktop\\test\\Opus\\plc_out.wav";
    fplc = fopen(plcFile, "wb");
    if (!fplc)
    {
        printf("can't open plc file.\n");
        return;
    }
#endif

    /* parameters setting */

#ifndef OPUS_CODESIZE
    init_wav_header(&header,channels,sample_rate);
    write_wav_header(fout,&header);
#ifdef HFP_PLC_DBG
    write_wav_header(fplc,&header);
#endif
#endif

    /* for MCPS calculate */
    frame_size = sample_rate / frame_ms;


    OPUS_CFG_DecParam opus_param;
    OPUS_CFG_DecParam* P_opus_param = &opus_param;
    P_opus_param->channels = channels;
    P_opus_param->sample_rate = sample_rate;
    P_opus_param->frame_size = frame_size;

    packet_loss_perc = 0;

    /* get decoder needed buffer size */
    decSize = tlka_opus_decoder_get_size(channels);

    scratchSize = tlka_opus_dec_get_scratch_size();
    scratch = malloc(scratchSize);

    /* opus decoder init */
    dec = malloc(decSize);
    if (dec == NULL)
    {
    	err = OPUS_ALLOC_FAIL;
    }

    err = tlka_opus_decoder_init(dec, P_opus_param);
    if (err != OPUS_OK)
    {
        printf("Cannot create decoder\n");
        return;
    }

#ifndef  OPUS_CODESIZE
    printf("decoder_st_size=%d\n", decSize);
    printf("Scratch Size :%d\n",scratchSize);

    printf("input file: %s\n", inFile);
    printf("output file: %s\n", outFile);
    printf("sample rate: %ld\n", sample_rate);
    printf("channels: %d\n", channels);
    printf("frame size: %d\n", frame_size);
#endif


#ifdef DUMP_IMTERMEDIATE
    dump_open();
#endif

    while (!stop)
    {
        frame_count++;
//        printf("frame:%d\n", frame_count);
#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
        printf("frame:%d\n", frame_count);
        //if (frame_count == 309)
            //printf("pause\n");
#endif

        /* read payload length*/
        num_read = fread(ch, 1, 4, fin);
        if (num_read != 4)
            break;
        len = char_to_int(ch);
        if (len > MAX_PACKET || len < 0)
        {
            fprintf(stderr, "Invalid payload length: %d\n", len);
            break;
        }

        /* read enc_final_range, not used by decoder */
        num_read = fread(ch, 1, 4, fin);
        if (num_read != 4)
            break;
        enc_final_range = char_to_int(ch);
        //printf("len%d\n",len);

        /* read a payload */
        num_read = fread(data, 1, len, fin);
        if (num_read != (size_t)len)
        {
            fprintf(stderr, "Ran out of input, "
                "expecting %d bytes got %d\n",
                len, (int)num_read);
            break;
        }

#ifdef HFP_PLC_DBG
       if((frame_count%30==3)||(frame_count%30==4)||(frame_count%30==5)||(frame_count%30==6))
#else
       if(0)
#endif
        {
            #if 1
            lost = 1;
    	   tlka_opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION_REQUEST, (&output_samples));
            #else
            lost++;
            tlka_opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION_REQUEST, (&output_samples));
            tlka_opus_decoder_ctl(dec, OPUS_SET_SKIP_PLC_REQUEST, 1);
            #endif
#ifdef HFP_PLC_DBG
            for(int i=0;i<MAX_FRAME_SIZE*MAX_CH;i++)
                plcout[i] = 0x3fff;
#endif
        }else{
            lost=0;
            #if 1
            output_samples = max_frame_size;
            #else
            output_samples = MAX_FRAME_SIZE;
            tlka_opus_decoder_ctl(dec, OPUS_SET_SKIP_PLC_REQUEST, 0);
            #endif
#ifdef HFP_PLC_DBG
            for(int i=0;i<MAX_FRAME_SIZE*MAX_CH;i++)
                plcout[i] = 0;
 #endif   
        }      
        // if((frame_count%30==6))
        // {
        //      tlka_opus_decoder_init_tt(dec, P_opus_param);
        // }        



        //tlka_opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION_REQUEST, (&output_samples));

        /* delay by one packet when using in-band FEC */

#if 0
        if((frame_count%30==3)||(frame_count%30==4)||(frame_count%30==5)||(frame_count%30==6))
       {
    	   lost = 1;
    	   tlka_opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION_REQUEST, (&output_samples));
       }
       else
       {
    	   output_samples = max_frame_size;
       }
#endif

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
        start_cyc = __nds__csrr(NDS_MCYCLE);
#endif

#if 1
        output_samples = tlka_opus_decode(dec, lost ? NULL : data, len, out, output_samples, 0,scratch);
#else
        output_samples = tlka_opus_decode(dec, data, len, out, output_samples, 0,scratch);
#endif


#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
        end_cyc = __nds__csrr(NDS_MCYCLE);
        if (lost)
        {
            if (max_plccyc < end_cyc - start_cyc)
            	max_plccyc = end_cyc - start_cyc;
            totalplccyc += end_cyc - start_cyc;
            plc_count++;
            printf("plc mcps=%.4f\n",(end_cyc - start_cyc)/frame_size*sample_rate/1000000.0);
        }
        else
        {
            if (max_cyc1 < end_cyc - start_cyc)
                max_cyc1 = end_cyc - start_cyc;
            totalcyc += end_cyc - start_cyc;
            printf("mcps=%.4f\n",(end_cyc - start_cyc)/frame_size*sample_rate/1000000.0);
        }


#endif
        if (output_samples > 0)
        {
            fwrite(out, sizeof(short) * channels, output_samples, fout);
#ifdef HFP_PLC_DBG
            fwrite(plcout, sizeof(short) * channels, output_samples, fplc);
#endif
        }
        else {
            fprintf(stderr, "error decoding frame: %ld\n", output_samples);
        }
        tot_samples += output_samples;

        /* compare final range encoder rng values of encoder and decoder */
        lost_prev = lost;
        count++;
    }

#ifdef DUMP_IMTERMEDIATE
    dump_close();
#endif

#ifndef OPUS_CODESIZE
    update_wav_header(&header,tot_samples);
    write_wav_header(fout,&header);
#ifdef HFP_PLC_DBG
    write_wav_header(fplc,&header);
    if (fplc)
        fclose(fplc);
#endif
#endif

    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
    printf("used stack %d \n", cal_used_stack(10 * 1024));
    printf("decoder_ave:%f decoder_max:%f\n", totalcyc / 1000000.0 * (sample_rate / frame_size) / (frame_count - plc_count), max_cyc1 / 1000000.0 * (sample_rate / frame_size));
    printf("PLC decoder_ave:%f decoder_max:%f\n", totalplccyc / 1000000.0 * (sample_rate / frame_size) / plc_count, max_plccyc / 1000000.0 * (sample_rate / frame_size));
#endif

    return;
}
#endif

#ifdef ENCODE_ONLY
void encode_main(int argc, char* argv[])
{
    int i, err;
    char* inFile, * outFile;
    FILE* fin = NULL;
    FILE* fout = NULL;
    OpusEncoder* enc = NULL;
    int encSize;
    int len;
    int frame_size;
	int channels;
    int frame_ms;
    opus_int32 bitrate_bps;
    opus_int32 sampling_rate;
    int complexity;
    int stop = 0;
    short in[MAX_FRAME_SIZE*MAX_CH];
    unsigned char data[MAX_PACKET];
    opus_uint32 enc_final_range;

    int curr_read = 0;
    int sweep_bps = 0;
    int sweep_max = 0, sweep_min = 0;
    int nb_modes_in_list = 0;
    int curr_mode = 0;
    int curr_mode_count = 0;
    int mode_switch_time = 48000;
    int nb_encoded = 0;
    int remaining = 0;
    unsigned char int_field[4];

    void* scratch = NULL;
    int scratchSize;

    WAV_HEADER header;

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
    write_stack(10 * 1024);
#endif

#if __riscv
    inFile = "D:\\Desktop\\test\\voice1_8k.wav";
    outFile = "D:\\Desktop\\test\\Opus\\voice1_8k_c5_a.opus";
    bitrate_bps = 12000;
    frame_ms = 50;
    complexity = 5;
#else
    inFile = argv[1];
    outFile = argv[2];
    bitrate_bps = atoi(argv[3]); 	/* CELT: 6kbps ~ 510kbps */
    frame_ms = atoi(argv[4]);       /* CELT: 1/0.0025 1/0.005 1/0.01 1/0.02 */
    complexity = atoi(argv[5]);
#endif

#if ENC_ONLY_SUPPORT_5MS
    if(frame_ms != 200)
    {   
        printf("only support 200 frame_rate.\n");
        return;
    }
#endif


     /* open input pcm file */
    #ifndef OPUS_CODESIZE
    fin = parse_wav_header(inFile, &header);
    #else
    fin = fopen(inFile, "rb");
    #endif
    if (!fin)
    {
        printf("can't open input file.\n");
        return;
    }

    /* open output bitstream file */
    fout = fopen(outFile, "wb");
    if (!fout)
    {
        printf("can't open output file.\n");
        return;
    }

    /* opus encoder init  */
    #if 0
    sampling_rate = 16000;
    channels = 1;
    #else
    sampling_rate = header.sampleRate;
    channels = header.numChannels;
    #endif
    
    frame_size = sampling_rate / frame_ms;

    if (sweep_max)
        sweep_min = bitrate_bps;

    OPUS_CFG_EncParam opus_encparam;
    OPUS_CFG_EncParam* P_opus_Encparam = &opus_encparam;
    P_opus_Encparam->channels = channels;
    P_opus_Encparam->sample_rate = sampling_rate;
    P_opus_Encparam->application = OPUS_APPLICATION_AUDIO;
    P_opus_Encparam->bitrate_bps = bitrate_bps;
    P_opus_Encparam->complexity = complexity;
    P_opus_Encparam->use_vbr = 0;

    /* get buffer size needed by opus encoder */
    encSize =  tlka_opus_encoder_get_size(channels);
    enc = malloc(encSize);
    if (enc == NULL)
    {
       err = OPUS_ALLOC_FAIL;
    }

    scratchSize = tlka_opus_enc_get_scratch_size(channels,frame_size,complexity);
    scratch = malloc(scratchSize);

    err = tlka_opus_encoder_init(enc, P_opus_Encparam);
    if (err != OPUS_OK)
    {
        printf("Cannot init encoder\n");
        return;
    }

    /* Mandatory set encoder parameters */
    tlka_opus_encoder_ctl(enc, OPUS_SET_FORCE_MODE_REQUEST, MODE_SILK_ONLY);
    //tlka_opus_encoder_ctl(enc, OPUS_SET_BITRATE_REQUEST, bitrate_bps);
    //tlka_opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH_REQUEST, OPUS_AUTO);
   tlka_opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY_REQUEST, (complexity));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_DTX_REQUEST, (0));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH_REQUEST, (16));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_VBR_REQUEST, (1));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT_REQUEST, (1));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC_REQUEST, (0));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS_REQUEST, (OPUS_AUTO));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC_REQUEST, (0));
    //tlka_opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&skip));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION_REQUEST, (OPUS_FRAMESIZE_ARG));
    //tlka_opus_encoder_ctl(enc, OPUS_SET_EXPERT_FRAME_DURATION_REQUEST, 5003);

#ifndef    OPUS_CODESIZE
    printf("Scratch Size :%d\n",scratchSize);
    printf("encoder_st_size=%d\n", encSize);
    printf("input file: %s\n", inFile);
    printf("output file: %s\n", outFile);
    printf("sample rate: %ld\n", sampling_rate);
    printf("bit rate: %ld\n", bitrate_bps);
    printf("complexity: %d\n", complexity);
    printf("frame size: %d\n", frame_size);
#endif

#ifdef DUMP_IMTERMEDIATE
    dump_open();
#endif

    /* Opus Encoder Process */
    while (!stop)
    {
        frame_count++;

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
        if (frame_count > 100)
        {
        	 break;
            // printf("pause\n");
        }
#endif

        /* read input */
        curr_read = (int)fread(in+remaining, sizeof(short) * channels, frame_size - remaining, fin);
        //curr_read = (int)fread(in, sizeof(short) * channels, frame_size - remaining, fin);
        if (curr_read + remaining < frame_size)
        {
            for (i = (curr_read + remaining) * channels; i < frame_size * channels; i++)
                in[i] = 0;

            stop = 1;
        }

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
        start_cyc = __nds__csrr(NDS_MCYCLE);
#endif

        len = tlka_opus_encode(enc, in, frame_size, data, MAX_PACKET,scratch);
        //printf("len:%d\n",len);

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
        end_cyc = __nds__csrr(NDS_MCYCLE);
        if (max_cyc < end_cyc - start_cyc)
            max_cyc = end_cyc - start_cyc;
        totalcyc += end_cyc - start_cyc;
        printf("frame %d, mcps=%.4f, len %d\n",frame_count, (end_cyc - start_cyc)/frame_size*sampling_rate/1000000.0, len);
#endif
        /* samples_per_frame * frame_count */
        nb_encoded = tlka_opus_packet_get_samples_per_frame(data, sampling_rate) * tlka_opus_packet_get_nb_frames(data, len);
        remaining = frame_size - nb_encoded;
    
        for (i = 0; i < remaining * channels; i++)
            in[i] = in[nb_encoded * channels + i];
       
        if (sweep_bps != 0)
        {
            bitrate_bps += sweep_bps;
            if (sweep_max)
            {
                if (bitrate_bps > sweep_max)
                    sweep_bps = -sweep_bps;
                else if (bitrate_bps < sweep_min)
                    sweep_bps = -sweep_bps;
            }
            /* safety */
            if (bitrate_bps < 1000)
                bitrate_bps = 1000;
            tlka_opus_encoder_ctl(enc, OPUS_SET_BITRATE_REQUEST, (bitrate_bps));
        }
        tlka_opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE_REQUEST, (&enc_final_range));
        if (len < 0)
        {
            printf("opus_encode() returned %d\n", len);
            return;
        }
        curr_mode_count += frame_size;
        if (curr_mode_count > mode_switch_time&& curr_mode < nb_modes_in_list - 1)
        {
            curr_mode++;
            curr_mode_count = 0;
        }

        /* write payload length */
        int_to_char(len, int_field);
        fwrite(int_field, 1, 4, fout);
        //printf("len:%d\n",len);

        /* write enc_final_range, un-used */
        int_to_char(enc_final_range, int_field);
        fwrite(int_field, 1, 4, fout);

        /* write stream data */
        fwrite(data, 1, len, fout);
    }

#ifdef DUMP_IMTERMEDIATE
    dump_close();
#endif

    if (fin)
        fclose(fin);
    if (fout)
        fclose(fout);

#if defined ANDES_PROFILE && !defined  OPUS_CODESIZE
    printf("used stack %d \n", cal_used_stack(10 * 1024));
    printf("encoder_ave:%f encoder_max:%f\n", totalcyc / 1000000.0 * (sampling_rate / frame_size) / frame_count, max_cyc / 1000000.0 * (sampling_rate / frame_size));
#endif

    return;
}

#endif

int main(int argc, char *argv[])
{
    printf("opus version: %x\n", tlka_opus_get_version());

#ifdef ENCODE_ONLY
    encode_main(argc, argv);
#endif

#ifdef DECODE_ONLY
    decode_main(argc, argv);
#endif

    return 0;
}

