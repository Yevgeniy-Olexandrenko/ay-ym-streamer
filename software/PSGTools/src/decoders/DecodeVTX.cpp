#include "DecodeVTX.h"
#include "lh5_decode/lh5_decode.h"

namespace
{
	const uint16_t VTXv2_SIG_AY = 0x7961; // "ay"
	const uint16_t VTXv2_SIG_YM = 0x6D79; // "ym"
}

////////////////////////////////////////////////////////////////////////////////

bool DecodeVTX::Open(Stream& stream)
{
	bool isDetected = false;
	std::ifstream fileStream;
	fileStream.open(stream.file, std::fstream::binary);

	if (fileStream)
	{
		fileStream.seekg(0, fileStream.end);
		uint32_t fileSize = (uint32_t)fileStream.tellg();

		if (fileSize > 20)
		{
			VTXHeader hdr; // data size here
			fileStream.seekg(0, fileStream.beg);
			fileStream.read((char*)(&hdr), sizeof(hdr));

			Chip::Model chipType(Chip::Model::Unknown);
			if (hdr.signature == VTXv2_SIG_AY) chipType = Chip::Model::AY;
			if (hdr.signature == VTXv2_SIG_YM) chipType = Chip::Model::YM;

			if (chipType != Chip::Model::Unknown)
			{
				stream.chip.model(chipType);

				if (hdr.stereo == VTXStereo::MONO) stream.chip.channels(Chip::Channels::MONO);
				if (hdr.stereo == VTXStereo::ABC ) stream.chip.channels(Chip::Channels::ABC);
				if (hdr.stereo == VTXStereo::ACB ) stream.chip.channels(Chip::Channels::ACB);

				stream.chip.freqValue(hdr.chipFreq);
				stream.playback.frameRate(hdr.frameFreq);

				auto GetTextProperty = [](std::ifstream& stream)
				{
					std::string str;
					if (stream) std::getline(stream, str, char(0x00));
					return str;
				};

				// read all properties to move forward on stream
				std::string title = GetTextProperty(fileStream);
				std::string author = GetTextProperty(fileStream);
				std::string program = GetTextProperty(fileStream); // store in extras
				std::string tracker = GetTextProperty(fileStream); // store in extras
				std::string comment = GetTextProperty(fileStream);

				stream.info.title(title);
				stream.info.artist(author);
				stream.info.comment(comment);
				stream.info.type("VTX stream");

				// unpack frames data
				uint32_t packedSize = fileSize - (uint32_t)fileStream.tellg();
				uint8_t* packedData = new uint8_t[packedSize];

				if (fileStream.read((char*)packedData, packedSize))
				{
					uint32_t depackedSize = hdr.dataSize;
					m_data = new uint8_t[depackedSize];

					if (lh5_decode(packedData, m_data, depackedSize, packedSize)) 
						isDetected = true;
					else 
						delete[] m_data;
				}
				delete[] packedData;

				// prepare information on frames
				m_frameCount = (hdr.dataSize / 14);
				m_loopFrame  = hdr.loop;
				m_nextFrame  = 0;
			}
		}
		fileStream.close();
	}
	return isDetected;
}

bool DecodeVTX::Decode(Frame& frame)
{
	uint8_t* dataPtr = m_data + m_nextFrame;
	m_nextFrame++;

	for (uint8_t r = 0; r < 14; r++)
	{
		uint8_t data = *dataPtr;
		if (r == Env_Shape)
		{
			if (data != 0xFF) 
				frame[r].first.override(data);
		}
		else
		{
			frame[r].first.update(data);
		}
		dataPtr += m_frameCount;
	}

	return (m_nextFrame < m_frameCount);
}

void DecodeVTX::Close(Stream& stream)
{
	if (m_loopFrame > 0)
		stream.loop.frameId(m_loopFrame);

	delete[] m_data;
}
