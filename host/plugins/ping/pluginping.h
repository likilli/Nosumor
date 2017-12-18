#ifndef PLUGINPING_H
#define PLUGINPING_H

#include <QtWidgets>
#include "plugin.h"

class PluginPing : public Plugin
{
public:
	PluginPing() : Plugin() {}
	~PluginPing() {}

	virtual std::string name() const {return "Ping-Pong";}
	virtual void *pluginWidget(hid_device *dev, hid_device_info *info, void *parent = nullptr);
};

#endif // PLUGINPING_H