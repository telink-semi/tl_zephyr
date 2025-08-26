/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <opus_api.h>
#include <zephyr/sys/slist.h>

#include <stdlib.h>

#include <ipc/ipc_based_driver.h>

#define LOG_LEVEL CONFIG_TELINK_OTBR_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(otbr_ext);

#define OPUS_MAX_FRAME_SIZE 1275

enum {
	IPC_DISPATCHER_OPUS_REMOTE_CHECK = IPC_OPUS_REMOTE,
	IPC_DISPATCHER_OPUS_REMOTE_DECODER_MK,
	IPC_DISPATCHER_OPUS_REMOTE_DECODER_RM,
	IPC_DISPATCHER_OPUS_REMOTE_DECODER_INP,
	IPC_DISPATCHER_OPUS_REMOTE_DECODER_OUT,
	IPC_DISPATCHER_OPUS_REMOTE_ENCODER_MK,
	IPC_DISPATCHER_OPUS_REMOTE_ENCODER_RM,
	IPC_DISPATCHER_OPUS_REMOTE_ENCODER_INP,
	IPC_DISPATCHER_OPUS_REMOTE_ENCODER_OUT
};

struct opus_decoder_param {
	uint8_t channels;
	uint16_t sample_rate;
	uint16_t samples_per_frame;
};

struct opus_encoder_param {
	uint8_t channels;
	uint16_t sample_rate;
	uint16_t samples_per_frame;
	enum {
		OPUS_REMOTE_APP_VOIP,
		OPUS_REMOTE_APP_AUDIO,
		OPUS_REMOTE_LOWDELAY

	} application;
	uint32_t bit_rate;
	uint8_t complexity;
	bool use_vbr;
};

struct opus_data {
	uint8_t *data;
	size_t length;
};

struct opus_decoder {
	sys_snode_t node;
	uint8_t id;
	OpusDecoder *decoder;
	void *scratch;
	struct opus_data inp;
	opus_int16 *out;
	struct k_work work;
	struct opus_decoder_param param;
};

struct opus_encoder {
	sys_snode_t node;
	uint8_t id;
	OpusEncoder *encoder;
	void *scratch;
	opus_int16 *inp;
	struct opus_data out;
	struct k_work work;
	struct opus_encoder_param param;
};

struct opus_coder_data {
	struct k_work_q work_q;
	sys_slist_t decoders;
	sys_slist_t encoders;
};

static void opus_decoder_work(struct k_work *item)
{
	struct opus_decoder *dec = CONTAINER_OF(item, struct opus_decoder, work);

	int frames = tlka_opus_decode(dec->decoder, dec->inp.data, dec->inp.length, &dec->out[8],
				      dec->param.samples_per_frame, 0, dec->scratch);

	if (frames != dec->param.samples_per_frame) {
		LOG_ERR("opus decode error");
	}

	uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_OPUS_REMOTE_DECODER_OUT, dec->id);
	uint32_t out_data_len =
		dec->param.samples_per_frame * dec->param.channels * sizeof(opus_int16);
	uint8_t *p_out = (uint8_t *)dec->out;

	IPC_DISPATCHER_PACK_FIELD(p_out, id);
	IPC_DISPATCHER_PACK_FIELD(p_out, out_data_len);
	if (ipc_dispatcher_send(dec->out, out_data_len + 8) != out_data_len + 8) {
		LOG_ERR("%s response fail", __func__);
	}
}

static struct opus_decoder *opus_decoder_search(sys_slist_t *decoders, uint8_t id)
{
	bool found = false;
	struct opus_decoder *dec;

	SYS_SLIST_FOR_EACH_CONTAINER(decoders, dec, node) {
		if (dec->id == id) {
			found = true;
			break;
		}
	}
	return found ? dec : NULL;
}

static int opus_decoder_mk(sys_slist_t *decoders, struct opus_decoder_param *dec_param)
{
	LOG_DBG("%s ch: %u, sr: %u, spf: %u", __func__, dec_param->channels, dec_param->sample_rate,
		dec_param->samples_per_frame);

	int result = -ENOMEM;
	struct opus_decoder *dec = NULL;

	do {
		dec = malloc(sizeof(struct opus_decoder));
		if (!dec) {
			LOG_ERR("opus can't allocate decoder item");
			break;
		}

		dec->decoder = NULL;
		dec->scratch = NULL;
		dec->inp.data = NULL;
		dec->out = NULL;

		dec->decoder = malloc(tlka_opus_decoder_get_size(dec_param->channels));
		if (!dec->decoder) {
			LOG_ERR("opus can't allocate decoder data");
			break;
		}
		dec->scratch = malloc(tlka_opus_dec_get_scratch_size());
		if (!dec->scratch) {
			LOG_ERR("opus can't allocate decoder scratch");
			break;
		}
		dec->inp.data = malloc(OPUS_MAX_FRAME_SIZE);
		if (!dec->inp.data) {
			LOG_ERR("opus can't allocate decoder input");
			break;
		}
		dec->inp.length = 0;
		/* 8 - extra bytes for IPC: ID, length */
		dec->out = malloc(8 + dec_param->samples_per_frame * dec_param->channels *
					      sizeof(opus_int16));
		if (!dec->out) {
			LOG_ERR("opus can't allocate decoder output");
			break;
		}

		result = -EINVAL;
		OPUS_CFG_DecParam opus_dec_param = {.channels = dec_param->channels,
						    .sample_rate = dec_param->sample_rate,
						    .frame_size = dec_param->samples_per_frame};

		if (tlka_opus_decoder_init(dec->decoder, &opus_dec_param) != OPUS_OK) {
			LOG_ERR("opus can't init decoder");
			break;
		}

		result = -ENOMEM;
		for (int i = 0; i <= 0xff; i++) {
			if (!opus_decoder_search(decoders, (uint8_t)i)) {
				result = i;
				break;
			}
		}
		if (result < 0) {
			LOG_ERR("opus can't assign decoder id");
			break;
		}

		dec->id = (uint8_t)result;
		k_work_init(&dec->work, opus_decoder_work);
		dec->param = *dec_param;
		sys_slist_append(decoders, &dec->node);

		LOG_DBG("%s done %d", __func__, result);
	} while (0);

	if (result < 0) {
		if (dec) {
			if (dec->decoder) {
				free(dec->decoder);
			}
			if (dec->scratch) {
				free(dec->scratch);
			}
			if (dec->inp.data) {
				free(dec->inp.data);
			}
			if (dec->out) {
				free(dec->out);
			}
			free(dec);
		}
	}
	return result;
}

static void opus_decoder_rm(sys_slist_t *decoders, uint8_t id)
{
	bool rm_elem = false;
	struct opus_decoder *dec;
	sys_snode_t *prev_node = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(decoders, dec, node) {
		if (dec->id == id) {
			rm_elem = true;
			sys_slist_remove(decoders, prev_node, &dec->node);
			break;
		}
		prev_node = &dec->node;
	}

	if (rm_elem) {
		if (dec->decoder) {
			free(dec->decoder);
		}
		if (dec->scratch) {
			free(dec->scratch);
		}
		if (dec->inp.data) {
			free(dec->inp.data);
		}
		if (dec->out) {
			free(dec->out);
		}
		free(dec);
	}
}

static void opus_encoder_work(struct k_work *item)
{
	struct opus_encoder *enc = CONTAINER_OF(item, struct opus_encoder, work);

	enc->out.length = tlka_opus_encode(enc->encoder, enc->inp, enc->param.samples_per_frame,
					   &enc->out.data[12], OPUS_MAX_FRAME_SIZE, enc->scratch);

	uint32_t enc_final_range = 0;
	uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_OPUS_REMOTE_ENCODER_OUT, enc->id);
	uint8_t *p_out = enc->out.data;

	tlka_opus_encoder_ctl(enc->encoder, OPUS_GET_FINAL_RANGE_REQUEST, &enc_final_range);
	IPC_DISPATCHER_PACK_FIELD(p_out, id);
	IPC_DISPATCHER_PACK_FIELD(p_out, enc_final_range);
	IPC_DISPATCHER_PACK_FIELD(p_out, enc->out.length);
	if (ipc_dispatcher_send(enc->out.data, enc->out.length + 12) != enc->out.length + 12) {
		LOG_ERR("%s response fail", __func__);
	}
}

static struct opus_encoder *opus_encoder_search(sys_slist_t *encoders, uint8_t id)
{
	bool found = false;
	struct opus_encoder *enc;

	SYS_SLIST_FOR_EACH_CONTAINER(encoders, enc, node) {
		if (enc->id == id) {
			found = true;
			break;
		}
	}
	return found ? enc : NULL;
}

static int opus_encoder_mk(sys_slist_t *encoders, struct opus_encoder_param *enc_param)
{
	LOG_DBG("%s ch: %u, sr: %u, spf: %u, app: %u, br: %u, comp: %u, vbr: %u", __func__,
		enc_param->channels, enc_param->sample_rate, enc_param->samples_per_frame,
		enc_param->application, enc_param->bit_rate, enc_param->complexity,
		enc_param->use_vbr);

	int result = -ENOMEM;
	struct opus_encoder *enc = NULL;

	do {
		enc = malloc(sizeof(struct opus_encoder));
		if (!enc) {
			LOG_ERR("opus can't allocate encoder item");
			break;
		}

		enc->encoder = NULL;
		enc->scratch = NULL;
		enc->inp = NULL;
		enc->out.data = NULL;

		enc->encoder = malloc(tlka_opus_encoder_get_size(enc_param->channels));
		if (!enc->encoder) {
			LOG_ERR("opus can't allocate encoder data");
			break;
		}
		enc->scratch = malloc(tlka_opus_enc_get_scratch_size(
			enc_param->channels, enc_param->samples_per_frame, enc_param->complexity));
		if (!enc->scratch) {
			LOG_ERR("opus can't allocate encoder scratch");
			break;
		}
		enc->inp = malloc(enc_param->samples_per_frame * enc_param->channels *
				  sizeof(opus_int16));
		if (!enc->inp) {
			LOG_ERR("opus can't allocate encoder input");
			break;
		}
		/* 12 - extra bytes for IPC: ID, final range, length */
		enc->out.data = malloc(12 + OPUS_MAX_FRAME_SIZE);
		if (!enc->out.data) {
			LOG_ERR("opus can't allocate encoder output");
			break;
		}
		enc->out.length = 0;

		result = -EINVAL;
		OPUS_CFG_EncParam opus_enc_param = {
			.channels = enc_param->channels,
			.sample_rate = enc_param->sample_rate,
			.application = enc_param->application == OPUS_REMOTE_APP_VOIP
					       ? OPUS_APPLICATION_VOIP
				       : enc_param->application == OPUS_REMOTE_LOWDELAY
					       ? OPUS_APPLICATION_RESTRICTED_LOWDELAY
					       : OPUS_APPLICATION_AUDIO,
			.complexity = enc_param->complexity,
			.bitrate_bps = enc_param->bit_rate,
			.use_vbr = enc_param->use_vbr};

		if (tlka_opus_encoder_init(enc->encoder, &opus_enc_param) != OPUS_OK) {
			LOG_ERR("opus can't init encoder");
			break;
		}

		tlka_opus_encoder_ctl(enc->encoder, OPUS_SET_FORCE_MODE_REQUEST, MODE_SILK_ONLY);
		tlka_opus_encoder_ctl(enc->encoder, OPUS_SET_COMPLEXITY_REQUEST,
				      opus_enc_param.complexity);

		result = -ENOMEM;
		for (int i = 0; i <= 0xff; i++) {
			if (!opus_encoder_search(encoders, (uint8_t)i)) {
				result = i;
				break;
			}
		}
		if (result < 0) {
			LOG_ERR("opus can't assign encoder id");
			break;
		}

		enc->id = (uint8_t)result;
		k_work_init(&enc->work, opus_encoder_work);
		enc->param = *enc_param;
		sys_slist_append(encoders, &enc->node);

		LOG_DBG("%s done %d", __func__, result);
	} while (0);

	if (result < 0) {
		if (enc) {
			if (enc->encoder) {
				free(enc->encoder);
			}
			if (enc->scratch) {
				free(enc->scratch);
			}
			if (enc->inp) {
				free(enc->inp);
			}
			if (enc->out.data) {
				free(enc->out.data);
			}
			free(enc);
		}
	}
	return result;
}

static void opus_encoder_rm(sys_slist_t *encoders, uint8_t id)
{
	bool rm_elem = false;
	struct opus_encoder *enc;
	sys_snode_t *prev_node = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(encoders, enc, node) {
		if (enc->id == id) {
			rm_elem = true;
			sys_slist_remove(encoders, prev_node, &enc->node);
			break;
		}
		prev_node = &enc->node;
	}

	if (rm_elem) {
		if (enc->encoder) {
			free(enc->encoder);
		}
		if (enc->scratch) {
			free(enc->scratch);
		}
		if (enc->inp) {
			free(enc->inp);
		}
		if (enc->out.data) {
			free(enc->out.data);
		}
		free(enc);
	}
}

static void opus_decoder_remove(const void *data, size_t len, void *param)
{
	LOG_DBG("%s", __func__);

	struct opus_coder_data *coder_data = param;
	const uint8_t *p_inp = data;
	int result;
	uint32_t id;

	IPC_DISPATCHER_UNPACK_FIELD(p_inp, id);
	if (len == sizeof(id)) {
		uint8_t decoder_id = (uint8_t)(id & 0xff);
		struct opus_decoder *dec = opus_decoder_search(&coder_data->decoders, decoder_id);

		if (dec) {
			if (!k_work_busy_get(&dec->work)) {
				ipc_dispatcher_rm(IPC_DISPATCHER_MK_ID(
					IPC_DISPATCHER_OPUS_REMOTE_DECODER_RM, decoder_id));
				ipc_dispatcher_rm(IPC_DISPATCHER_MK_ID(
					IPC_DISPATCHER_OPUS_REMOTE_DECODER_INP, decoder_id));
				opus_decoder_rm(&coder_data->decoders, decoder_id);
				result = 0;
			} else {
				LOG_ERR("%s busy", __func__);
				result = -EBUSY;
			}
		} else {
			LOG_ERR("%s bad instance", __func__);
			result = -EINVAL;
		}
	} else {
		LOG_ERR("%s malformed frame", __func__);
		result = -EINVAL;
	}

	uint8_t out[sizeof(id) + sizeof(result)];
	uint8_t *p_out = out;

	IPC_DISPATCHER_PACK_FIELD(p_out, id);
	IPC_DISPATCHER_PACK_FIELD(p_out, result);
	if (ipc_dispatcher_send(out, sizeof(out)) != sizeof(out)) {
		LOG_ERR("%s response fail", __func__);
	}
}

static void opus_decoder_input(const void *data, size_t len, void *param)
{
	LOG_DBG("%s", __func__);

	struct opus_coder_data *coder_data = param;
	const uint8_t *p_inp = data;
	int result;
	uint32_t id;
	uint32_t input_data_length;

	IPC_DISPATCHER_UNPACK_FIELD(p_inp, id);
	if (len >= sizeof(id) + sizeof(input_data_length)) {
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, input_data_length);
		if (len == sizeof(id) + sizeof(input_data_length) + input_data_length) {
			uint8_t decoder_id = (uint8_t)(id & 0xff);
			struct opus_decoder *dec =
				opus_decoder_search(&coder_data->decoders, decoder_id);

			if (dec) {
				if (input_data_length) {
					if (!k_work_busy_get(&dec->work)) {
						dec->inp.length = input_data_length;
						IPC_DISPATCHER_UNPACK_ARRAY(p_inp, dec->inp.data,
									    dec->inp.length);
						k_work_submit_to_queue(&coder_data->work_q,
								       &dec->work);
						result = 0;
					} else {
						LOG_ERR("%s busy", __func__);
						result = -EBUSY;
					}
				} else {
					LOG_ERR("%s bad input", __func__);
					result = -EINVAL;
				}
			} else {
				LOG_ERR("%s bad instance", __func__);
				result = -EINVAL;
			}
		} else {
			LOG_ERR("%s malformed frame", __func__);
			result = -EINVAL;
		}
	} else {
		LOG_ERR("%s malformed frame", __func__);
		result = -EINVAL;
	}

	uint8_t out[sizeof(id) + sizeof(result)];
	uint8_t *p_out = out;

	IPC_DISPATCHER_PACK_FIELD(p_out, id);
	IPC_DISPATCHER_PACK_FIELD(p_out, result);
	if (ipc_dispatcher_send(out, sizeof(out)) != sizeof(out)) {
		LOG_ERR("%s response fail", __func__);
	}
}

static void opus_decoder_create(const void *data, size_t len, void *param)
{
	LOG_DBG("%s", __func__);

	struct opus_coder_data *coder_data = param;
	struct opus_decoder_param dec_param;

	do {
		if (len != sizeof(uint32_t) + sizeof(dec_param.channels) +
				   sizeof(dec_param.sample_rate) +
				   sizeof(dec_param.samples_per_frame)) {
			LOG_ERR("%s malformed frame", __func__);
			break;
		}
		const uint8_t *p_inp = data;

		p_inp += sizeof(uint32_t);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, dec_param.channels);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, dec_param.sample_rate);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, dec_param.samples_per_frame);

		int decoder = opus_decoder_mk(&coder_data->decoders, &dec_param);

		if (decoder >= 0) {
			ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(
						   IPC_DISPATCHER_OPUS_REMOTE_DECODER_RM, decoder),
					   opus_decoder_remove, coder_data);
			ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(
						   IPC_DISPATCHER_OPUS_REMOTE_DECODER_INP, decoder),
					   opus_decoder_input, coder_data);
		}

		uint8_t out[sizeof(uint32_t) + sizeof(decoder)];
		uint8_t *p_out = out;
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_OPUS_REMOTE_DECODER_MK, 0);

		IPC_DISPATCHER_PACK_FIELD(p_out, id);
		IPC_DISPATCHER_PACK_FIELD(p_out, decoder);
		if (ipc_dispatcher_send(out, sizeof(out)) != sizeof(out)) {
			LOG_ERR("%s response fail", __func__);
		}
	} while (0);
}

static void opus_encoder_remove(const void *data, size_t len, void *param)
{
	LOG_DBG("%s", __func__);

	struct opus_coder_data *coder_data = param;
	const uint8_t *p_inp = data;
	int result;
	uint32_t id;

	IPC_DISPATCHER_UNPACK_FIELD(p_inp, id);
	if (len == sizeof(id)) {
		uint8_t encoder_id = (uint8_t)(id & 0xff);
		struct opus_encoder *enc = opus_encoder_search(&coder_data->encoders, encoder_id);

		if (enc) {
			if (!k_work_busy_get(&enc->work)) {
				ipc_dispatcher_rm(IPC_DISPATCHER_MK_ID(
					IPC_DISPATCHER_OPUS_REMOTE_ENCODER_RM, encoder_id));
				ipc_dispatcher_rm(IPC_DISPATCHER_MK_ID(
					IPC_DISPATCHER_OPUS_REMOTE_ENCODER_INP, encoder_id));
				opus_encoder_rm(&coder_data->encoders, encoder_id);
				result = 0;
			} else {
				LOG_ERR("%s busy", __func__);
				result = -EBUSY;
			}
		} else {
			LOG_ERR("%s bad instance", __func__);
			result = -EINVAL;
		}
	} else {
		LOG_ERR("%s malformed frame", __func__);
		result = -EINVAL;
	}

	uint8_t out[sizeof(id) + sizeof(result)];
	uint8_t *p_out = out;

	IPC_DISPATCHER_PACK_FIELD(p_out, id);
	IPC_DISPATCHER_PACK_FIELD(p_out, result);
	if (ipc_dispatcher_send(out, sizeof(out)) != sizeof(out)) {
		LOG_ERR("%s response fail", __func__);
	}
}

static void opus_encoder_input(const void *data, size_t len, void *param)
{
	LOG_DBG("%s", __func__);

	struct opus_coder_data *coder_data = param;
	const uint8_t *p_inp = data;
	int result;
	uint32_t id;
	uint32_t input_data_length;

	IPC_DISPATCHER_UNPACK_FIELD(p_inp, id);
	if (len >= sizeof(id) + sizeof(input_data_length)) {
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, input_data_length);
		if (len == sizeof(id) + sizeof(input_data_length) + input_data_length) {
			uint8_t encoder_id = (uint8_t)(id & 0xff);
			struct opus_encoder *enc =
				opus_encoder_search(&coder_data->encoders, encoder_id);

			if (enc) {
				if (input_data_length == enc->param.samples_per_frame *
								 enc->param.channels *
								 sizeof(opus_int16)) {
					if (!k_work_busy_get(&enc->work)) {
						IPC_DISPATCHER_UNPACK_ARRAY(p_inp, enc->inp,
									    input_data_length);
						k_work_submit_to_queue(&coder_data->work_q,
								       &enc->work);
						result = 0;
					} else {
						LOG_ERR("%s busy", __func__);
						result = -EBUSY;
					}
				} else {
					LOG_ERR("%s bad input", __func__);
					result = -EINVAL;
				}
			} else {
				LOG_ERR("%s bad instance", __func__);
				result = -EINVAL;
			}
		} else {
			LOG_ERR("%s malformed frame", __func__);
			result = -EINVAL;
		}
	} else {
		LOG_ERR("%s malformed frame", __func__);
		result = -EINVAL;
	}

	uint8_t out[sizeof(id) + sizeof(result)];
	uint8_t *p_out = out;

	IPC_DISPATCHER_PACK_FIELD(p_out, id);
	IPC_DISPATCHER_PACK_FIELD(p_out, result);
	if (ipc_dispatcher_send(out, sizeof(out)) != sizeof(out)) {
		LOG_ERR("%s response fail", __func__);
	}
}

static void opus_encoder_create(const void *data, size_t len, void *param)
{
	LOG_DBG("%s", __func__);

	struct opus_coder_data *coder_data = param;
	struct opus_encoder_param enc_param;

	do {
		if (len != sizeof(uint32_t) + sizeof(enc_param.channels) +
				   sizeof(enc_param.sample_rate) +
				   sizeof(enc_param.samples_per_frame) +
				   sizeof(enc_param.application) + sizeof(enc_param.bit_rate) +
				   sizeof(enc_param.complexity) + sizeof(enc_param.use_vbr)) {
			LOG_ERR("%s malformed frame", __func__);
			break;
		}
		const uint8_t *p_inp = data;

		p_inp += sizeof(uint32_t);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, enc_param.channels);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, enc_param.sample_rate);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, enc_param.samples_per_frame);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, enc_param.application);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, enc_param.bit_rate);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, enc_param.complexity);
		IPC_DISPATCHER_UNPACK_FIELD(p_inp, enc_param.use_vbr);

		int encoder = opus_encoder_mk(&coder_data->encoders, &enc_param);

		if (encoder >= 0) {
			ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(
						   IPC_DISPATCHER_OPUS_REMOTE_ENCODER_RM, encoder),
					   opus_encoder_remove, coder_data);
			ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(
						   IPC_DISPATCHER_OPUS_REMOTE_ENCODER_INP, encoder),
					   opus_encoder_input, coder_data);
		}

		uint8_t out[sizeof(uint32_t) + sizeof(encoder)];
		uint8_t *p_out = out;
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_OPUS_REMOTE_ENCODER_MK, 0);

		IPC_DISPATCHER_PACK_FIELD(p_out, id);
		IPC_DISPATCHER_PACK_FIELD(p_out, encoder);
		if (ipc_dispatcher_send(out, sizeof(out)) != sizeof(out)) {
			LOG_ERR("%s response fail", __func__);
		}
	} while (0);
}

static void opus_check(const void *data, size_t len, void *param)
{
	LOG_DBG("%s", __func__);

	if (ipc_dispatcher_send(data, len) != len) {
		LOG_ERR("%s response fail", __func__);
	}
}

static int opus_init_process(void)
{
	LOG_DBG("%s", __func__);

	static K_KERNEL_STACK_DEFINE(opus_work_q_stack,
				     CONFIG_TELINK_OPUS_REMOTE_THREAD_STACK_SIZE);
	static struct opus_coder_data coder_data;

	struct k_work_queue_config opus_work_q_config = {
		.name = "opus_remte_workq", .no_yield = false, .essential = false};

	k_work_queue_start(&coder_data.work_q, opus_work_q_stack,
			   K_KERNEL_STACK_SIZEOF(opus_work_q_stack),
			   CONFIG_TELINK_OPUS_REMOTE_THREAD_PRIORITY, &opus_work_q_config);

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_OPUS_REMOTE_DECODER_MK, 0),
			   opus_decoder_create, &coder_data);
	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_OPUS_REMOTE_ENCODER_MK, 0),
			   opus_encoder_create, &coder_data);
	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_OPUS_REMOTE_CHECK, 0), opus_check,
			   NULL);
	return 0;
}

SYS_INIT(opus_init_process, APPLICATION, CONFIG_TELINK_OPUS_REMOTE_INIT_PRIO);
