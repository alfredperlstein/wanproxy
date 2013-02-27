/*
 * Copyright (c) 2009-2013 Juli Mallett. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	CONFIG_CONFIG_CLASS_LOG_MASK_H
#define	CONFIG_CONFIG_CLASS_LOG_MASK_H

#include <config/config_type_log_level.h>
#include <config/config_type_string.h>

class ConfigClassLogMask : public ConfigClass {
	struct Instance : public ConfigClassInstance {
		std::string regex_;
		Log::Priority mask_;

		bool activate(const ConfigObject *);
	};
public:
	ConfigClassLogMask(void)
	: ConfigClass("log-mask", new ConstructorFactory<ConfigClassInstance, Instance>)
	{
		add_member("regex", &config_type_string, &Instance::regex_);
		add_member("mask", &config_type_log_level, &Instance::mask_);
	}

	~ConfigClassLogMask()
	{ }
};

extern ConfigClassLogMask config_class_log_mask;

#endif /* !CONFIG_CONFIG_CLASS_LOG_MASK_H */
