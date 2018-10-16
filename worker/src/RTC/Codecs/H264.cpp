#define MS_CLASS "RTC::Codecs::H264"
// #define MS_LOG_DEV

#include "RTC/Codecs/H264.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace Codecs
	{
		H264::PayloadDescriptor* H264::Parse(uint8_t* data, size_t len)
		{
			MS_TRACE();

			(void)data;
			std::unique_ptr<PayloadDescriptor> payloadDescriptor(new PayloadDescriptor());

			if (len < 2)
				return nullptr;

			uint8_t nal = *data & 0x1F;

			/** Single NAL unit packet **/

			// nal == 5: IDR (instantaneous decoding picture).
			// nal == 7: PSP (sequence parameter set).
			if (nal == 5 || nal == 7)
			{
				payloadDescriptor->isKeyFrame = true;
			}

			/** Aggreation packet **/

			// STAP-A.
			else if (nal == 24)
			{
				nal = *(data + 3) & 0x1F;

				if (nal == 5 || nal == 7)
					payloadDescriptor->isKeyFrame = true;
			}
			// FU-A, FU-B.
			else if (nal == 28 || nal == 29)
			{
				nal              = *(data + 1) & 0x1F;
				uint8_t startBit = *(data + 1) & 0x80;

				if ((nal == 5 || nal == 7) && startBit == 128)
					payloadDescriptor->isKeyFrame = true;
			}

			return payloadDescriptor.release();
		}

		void H264::PayloadDescriptor::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<PayloadDescriptor>");
			MS_DUMP("  isKeyFrame      : %s", this->isKeyFrame ? "true" : "false");
			MS_DUMP("</PayloadDescriptor>");
		}

		H264::PayloadDescriptorHandler::PayloadDescriptorHandler(H264::PayloadDescriptor* payloadDescriptor)
		{
			this->payloadDescriptor.reset(payloadDescriptor);
		}

		void H264::ProcessRtpPacket(RTC::RtpPacket* packet)
		{
			MS_TRACE();

			auto data = packet->GetPayload();
			auto len  = packet->GetPayloadLength();

			PayloadDescriptor* payloadDescriptor = Parse(data, len);

			if (!payloadDescriptor)
				return;

			PayloadDescriptorHandler* payloadDescriptorHandler =
			  new PayloadDescriptorHandler(payloadDescriptor);

			packet->SetPayloadDescriptorHandler(payloadDescriptorHandler);
		}
	} // namespace Codecs
} // namespace RTC