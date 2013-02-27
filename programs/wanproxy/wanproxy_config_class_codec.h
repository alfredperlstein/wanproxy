#ifndef	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CODEC_H
#define	PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CODEC_H

#include <config/config_type_int.h>

#include "wanproxy_codec.h"
#include "wanproxy_config_type_codec.h"
#include "wanproxy_config_type_compressor.h"

class WANProxyConfigClassCodec : public ConfigClass {
public:
	struct Instance : public ConfigClassInstance {
		WANProxyCodec codec_;
		WANProxyConfigCodec codec_type_;
		WANProxyConfigCompressor compressor_;
		intmax_t compressor_level_;

		intmax_t outgoing_to_codec_bytes_;
		intmax_t codec_to_outgoing_bytes_;
		intmax_t incoming_to_codec_bytes_;
		intmax_t codec_to_incoming_bytes_;

		Instance(void)
		: codec_(),
		  codec_type_(WANProxyConfigCodecNone),
		  compressor_(WANProxyConfigCompressorNone),
		  compressor_level_(0),
		  outgoing_to_codec_bytes_(0),
		  codec_to_outgoing_bytes_(0),
		  incoming_to_codec_bytes_(0),
		  codec_to_incoming_bytes_(0)
		{
			codec_.outgoing_to_codec_bytes_ = &outgoing_to_codec_bytes_;
			codec_.codec_to_outgoing_bytes_ = &codec_to_outgoing_bytes_;
			codec_.incoming_to_codec_bytes_ = &incoming_to_codec_bytes_;
			codec_.codec_to_incoming_bytes_ = &codec_to_incoming_bytes_;
		}

		bool activate(const ConfigObject *);
	};

	WANProxyConfigClassCodec(void)
	: ConfigClass("codec", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("codec", &wanproxy_config_type_codec, &Instance::codec_type_);
		add_member("compressor", &wanproxy_config_type_compressor, &Instance::compressor_);
		add_member("compressor_level", &config_type_int, &Instance::compressor_level_);

		add_member("outgoing_to_codec_bytes", &config_type_int, &Instance::outgoing_to_codec_bytes_);
		add_member("codec_to_outgoing_bytes", &config_type_int, &Instance::codec_to_outgoing_bytes_);
		add_member("incoming_to_codec_bytes", &config_type_int, &Instance::incoming_to_codec_bytes_);
		add_member("codec_to_incoming_bytes", &config_type_int, &Instance::codec_to_incoming_bytes_);
	}

	~WANProxyConfigClassCodec()
	{ }
};

extern WANProxyConfigClassCodec wanproxy_config_class_codec;

#endif /* !PROGRAMS_WANPROXY_WANPROXY_CONFIG_CLASS_CODEC_H */
