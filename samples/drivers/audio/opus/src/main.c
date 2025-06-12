/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
// #include <zephyr/drivers/audio/audio_codec.h>
#include <zephyr/logging/log.h>
#include <zephyr/audio/codec.h>
#include <opus/tlka_opus_api.h>

LOG_MODULE_REGISTER(audio_test, LOG_LEVEL_INF);

#define CODEC_NODE DT_NODELABEL(w91_audio)
/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      4 // CONFIG_I2S_INIT_BUFFERS
#define NUMBER_OF_CHANNELS (2U)
#define TIMEOUT             (2000U)
#define SAMPLE_FREQUENCY    16000 // CONFIG_SAMPLE_FREQ
#define SAMPLE_BIT_WIDTH    (16U)
#define BYTES_PER_SAMPLE    sizeof(int16_t)

// Lead to RAM overflowed by 62952 bytes
#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
// #define BLOCK_COUNT (INITIAL_BLOCKS + 32)
// K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);


int main(void)
{
	LOG_INF("TLSR9118 audio test start");

	printk("OPUS version: %d", tlka_opus_get_version()); // lib linking check

	const struct device *codec = DEVICE_DT_GET(CODEC_NODE);

	if (!device_is_ready(codec)) {
		LOG_ERR("Audio codec device not ready");
		return -1;
	}

	struct audio_codec_cfg audio_cfg;

	audio_cfg.dai_route = AUDIO_ROUTE_PLAYBACK;
	audio_cfg.dai_type = AUDIO_DAI_TYPE_I2S;
	audio_cfg.dai_cfg.i2s.word_size = SAMPLE_BIT_WIDTH;
	audio_cfg.dai_cfg.i2s.channels =  2;
	audio_cfg.dai_cfg.i2s.format = I2S_FMT_DATA_FORMAT_I2S;
	audio_cfg.dai_cfg.i2s.options = I2S_OPT_FRAME_CLK_MASTER;
	audio_cfg.dai_cfg.i2s.frame_clk_freq = SAMPLE_FREQUENCY;
	audio_cfg.dai_cfg.i2s.mem_slab = NULL;
	audio_cfg.dai_cfg.i2s.block_size = BLOCK_SIZE;

    int err = audio_codec_configure(codec, &audio_cfg);
    if (err) {
        LOG_ERR("Configure error: %d", err);
        return -1;
    }

	// if (audio_codec_start(codec, AUDIO_STREAM_PLAYBACK) != 0) {
	// 	LOG_ERR("Failed to start codec");
	// 	return;
	// }

	LOG_INF("Playback started");

	k_sleep(K_SECONDS(2));

	// audio_codec_stop(codec, AUDIO_STREAM_PLAYBACK);

	LOG_INF("Playback stopped");

	return 0;
}
