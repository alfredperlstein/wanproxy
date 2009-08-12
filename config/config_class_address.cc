#include <config/config_class.h>
#include <config/config_class_address.h>
#include <config/config_object.h>
#include <config/config_value.h>

ConfigClassAddress config_class_address;

bool
ConfigClassAddress::activate(ConfigObject *co)
{
	ConfigTypeAddressFamily *familyct;
	ConfigValue *familycv = co->get("family", &familyct);
	if (familycv == NULL)
		return (false);

	ConfigAddressFamily family;
	if (!familyct->get(familycv, &family))
		return (false);

	if (family == ConfigAddressFamilyIPv4) {
		if (co->has("path"))
			return (false);
		return (true);
	}

	if (family == ConfigAddressFamilyUnix) {
		if (co->has("host") || co->has("port"))
			return (false);
		return (true);
	}

	return (false);
}
