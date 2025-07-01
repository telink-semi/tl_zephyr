#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/audio/codec.h>
#include <zephyr/logging/log.h>
#include <ipc/ipc_based_driver.h>

LOG_MODULE_REGISTER(w91_audio, LOG_LEVEL_DBG);

#define DT_DRV_COMPAT telink_w91_audio

enum {
	IPC_DISPATCHER_AUDIO_INIT = IPC_DISPATCHER_AUDIO,
	IPC_DISPATCHER_AUDIO_CONFIG,
	IPC_DISPATCHER_AUDIO_START_OUTPUT,
	IPC_DISPATCHER_AUDIO_STOP_OUTPUT
};

/* Private driver data */
struct tlsr9118_audio_data {
    bool is_playing;

	// struct spi_context ctx;
	// struct spi_config config;
	// struct k_mutex mutex;
	struct ipc_based_driver ipc; /* ipc driver part */
};

struct w91_audio_config {
	// const struct pinctrl_dev_config *pcfg;
	uint8_t instance_id;
};

struct w91_audio_config_req {
	// uint8_t role;
	// uint8_t clk_src;
	// uint8_t clk_div_2mul;
	// uint8_t mode;
	// uint8_t data_io_format;
	// uint8_t bit_order;
};

/* APIs implementation: AUDIO codec configure */
static size_t pack_w91_audio_ipc_configure(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	// struct spi_w91_config_req *p_config_req = unpack_data;
	// size_t pack_data_len = sizeof(uint32_t) + sizeof(p_config_req->role) +
	// 	sizeof(p_config_req->clk_src) + sizeof(p_config_req->clk_div_2mul) +
	// 	sizeof(p_config_req->mode) + sizeof(p_config_req->data_io_format) +
	// 	sizeof(p_config_req->bit_order);

	// if (pack_data != NULL) {
	// 	uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_SPI_CONFIG, inst);

	// 	IPC_DISPATCHER_PACK_FIELD(pack_data, id);
	// 	IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->role);
	// 	IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->clk_src);
	// 	IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->clk_div_2mul);
	// 	IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->mode);
	// 	IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->data_io_format);
	// 	IPC_DISPATCHER_PACK_FIELD(pack_data, p_config_req->bit_order);
	// }

	return 0;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(w91_audio_ipc_configure);

/*
 * Called to configure the codec: sample rate, formats, etc.
 * TODO: replace LOG_DBG and arguments with sending IPC MSG_CODEC_CONFIG.
 */
static int tlsr9118_audio_configure(const struct device *dev,
                                     struct audio_codec_cfg *cfg)
{
	int err = -ETIMEDOUT;
	struct w91_audio_config_req config_req;
	uint8_t inst = ((struct w91_audio_config *)dev->config)->instance_id;

	/* TODO: setup ACTUAL config parameters*/

	struct ipc_based_driver *ipc_data = &((struct tlsr9118_audio_data *)dev->data)->ipc;

	IPC_DISPATCHER_HOST_SEND_DATA(ipc_data, inst,
		w91_audio_ipc_configure, &config_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	// if (!err) {
	// 	spi_w91_save_config(dev, config);
	// }
    LOG_DBG("Configured codec with sample rate %u",
            (unsigned int)cfg->dai_cfg.i2s.frame_clk_freq);

	return err;
}

/*
 * Called to start playback.
 * TODO: replace LOG_DBG and flag handling with sending IPC MSG_CODEC_START.
 */
static void tlsr9118_audio_start_output(const struct device *dev)
{
    struct tlsr9118_audio_data *data = dev->data;

    data->is_playing = true;
    LOG_DBG("Playback started");
}

/*
 * Called to stop playback.
 * TODO: replace LOG_DBG and flag handling with sending IPC MSG_CODEC_STOP.
 */
static void tlsr9118_audio_stop_output(const struct device *dev)
{
    struct tlsr9118_audio_data *data = dev->data;

    if (!data->is_playing) {
        return;
    }

    data->is_playing = false;
    LOG_DBG("Playback stopped");
}

/* Audio codec API declaration */
static const struct audio_codec_api tlsr9118_audio_driver_api = {
    // .configure    = tlsr9118_audio_configure,
    // .start_output = tlsr9118_audio_start_output,
    // .stop_output  = tlsr9118_audio_stop_output,
};

/* Driver instance definition */
static struct tlsr9118_audio_data tlsr9118_audio_driver_data;

DEVICE_DT_INST_DEFINE(0,
                      NULL,        /* No init function needed */
                      NULL,
                      &tlsr9118_audio_driver_data,
                      NULL,
                      POST_KERNEL,
                      CONFIG_AUDIO_CODEC_INIT_PRIORITY,
                      &tlsr9118_audio_driver_api);
