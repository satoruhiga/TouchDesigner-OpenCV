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

			ParAppendResult res = manager->appendString(p);
			assert(res == PARAMETER_APPEND_SUCCESS);
		}
	}
	
	void getGeneralInfo(TOP_GeneralInfo* ginfo) override
	{
		ginfo->cookEveryFrame = true;
		ginfo->cookEveryFrameIfAsked = true;
		ginfo->executeMode = OPENGL_FBO;
		ginfo->memPixelType = RGBA8Fixed;
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
		options.downloadType = OP_TOP_INPUT_DOWNLOAD_DELAYED;
		options.cpuMemPixelType = RGBA8Fixed;
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

extern "C" {
	DLLEXPORT int GetTOPAPIVersion(void) {
		return TOP_CPLUSPLUS_API_VERSION;
	}

	DLLEXPORT TOP_CPlusPlusBase* CreateTOPInstance(const OP_NodeInfo* info, TOP_Context* context) {
		return new NanomsgTOP(info);
	}

	DLLEXPORT void DestroyTOPInstance(TOP_CPlusPlusBase* instance, TOP_Context *context) {
		delete (NanomsgTOP*)instance;
	}
};
