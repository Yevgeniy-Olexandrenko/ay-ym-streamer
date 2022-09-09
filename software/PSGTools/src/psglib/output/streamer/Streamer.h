#pragma once

#include "output/Output.h"
#include "SerialPort.h"

class Streamer : public Output
{
public:
	Streamer(int comPortIndex);
	virtual ~Streamer();

protected:
	const std::string GetDeviceName() const override;

	bool OpenDevice() override;
	bool InitDstChip(const Chip& srcChip, Chip& dstChip) override;
	bool WriteToChip(int chip, const Data& data) override;
	void CloseDevice() override;

private:
	int m_portIndex;
	SerialPort m_port;
};
