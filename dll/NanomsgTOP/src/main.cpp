/* Shared Use License: This file is owned by Derivative Inc. (Derivative) and
 * can only be used, and/or modified for use, in conjunction with 
 * Derivative's TouchDesigner software, and only if you are a licensee who has
 * accepted Derivative's TouchDesigner license or assignment agreement (which
 * also govern the use of this file).  You may share a modified version of this
 * file with another authorized licensee of Derivative's TouchDesigner software.
 * Otherwise, no redistribution or sharing of this file, with or without
 * modification, is permitted.
 */

#include "TOP_CPlusPlusBase.h"

#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <memory>

#include "NanomsgIO/NanomsgIO.h"

class NanomsgTOP : public TOP_CPlusPlusBase
{
public:

	std::shared_ptr<NanomsgIO::Publisher> pub;
	std::string nanomsg_addr;

	NanomsgTOP(const OP_NodeInfo *info)
	{}

	void setupParameters(OP_ParameterManager* manager) override
	{
		{
			OP_StringParameter p;
			p.name = "Nanomsgaddress";
			p.label = "Nanomsg Address";

			p.defaultValue = "ipc://test";

			OP_ParAppendResult res = manager->appendString(p);
			assert(res == OP_ParAppendResult::Success);
		}
	}

	void getGeneralInfo(TOP_GeneralInfo* ginfo) override
	{
		ginfo->cookEveryFrame = true;
		ginfo->cookEveryFrameIfAsked = true;
		// ginfo->executeMode = OpenGL_FBO;
		ginfo->memPixelType = OP_CPUMemPixelType::RGBA32Float;
	}

	void execute(const TOP_OutputFormatSpecs* outputFormat, OP_Inputs* inputs, TOP_Context* context) override
	{
		if (inputs->getNumInputs() == 0)
			return;

		{
			const char* s = inputs->getParString("Nanomsgaddress");
			if (nanomsg_addr != s)
			{
				pub = std::make_shared<NanomsgIO::Publisher>();
				nanomsg_addr = s;

				if (!nanomsg_addr.empty())
					pub->bind(nanomsg_addr);
				else
					pub.reset();
			}
		}

		if (!pub) return;

		const OP_TOPInput* input = inputs->getInputTOP(0);

		OP_TOPInputDownloadOptions options;
		options.downloadType = OP_TOPInputDownloadType::Delayed;
		options.cpuMemPixelType = OP_CPUMemPixelType::RGBA8Fixed;
		options.verticalFlip = true;
		const uint8_t* src = (const uint8_t*)inputs->getTOPDataInCPUMemory(input, &options);
		if (!src) return;

		int w = input->width;
		int h = input->height;
		int npixels = w * h;
		size_t bytes_par_pixel = 4;

		size_t N = npixels * bytes_par_pixel;

		pub->send(src, N);
	}
};

// These functions are basic C function, which the DLL loader can find
// much easier than finding a C++ Class.
// The DLLEXPORT prefix is needed so the compile exports these functions from the .dll
// you are creating
extern "C"
{

DLLEXPORT
TOP_PluginInfo
GetTOPPluginInfo(void)
{
	TOP_PluginInfo info;
	// This must always be set to this constant
	info.apiVersion = TOPCPlusPlusAPIVersion;

	// Change this to change the executeMode behavior of this plugin.
	info.executeMode = TOP_ExecuteMode::CPUMemWriteOnly;

	return info;
}

DLLEXPORT
TOP_CPlusPlusBase*
CreateTOPInstance(const OP_NodeInfo* info, TOP_Context* context)
{
	// Return a new instance of your class every time this is called.
	// It will be called once per TOP that is using the .dll
	return new NanomsgTOP(info);
}

DLLEXPORT
void
DestroyTOPInstance(TOP_CPlusPlusBase* instance, TOP_Context *context)
{
	// Delete the instance here, this will be called when
	// Touch is shutting down, when the TOP using that instance is deleted, or
	// if the TOP loads a different DLL
	delete (NanomsgTOP*)instance;
}

};

