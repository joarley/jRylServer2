#ifndef _PING_H_
#define	_PING_H_

#include "../Buffer.h"
#include "PacketBase.h"
#include "../typedef.h"
#include "../Logger.h"

namespace Common {
namespace packet {

class Ping: public PacketBase{
public:
    Ping(Buffer_ptr buff);
	Ping();
    virtual ~Ping();
    
    static const uint8 PacketID = 0x59;
    static const uint16 PacketLength = 16;

    inline uint32 GetPingValue();
	inline void SetPingValue(uint32 pingValue);

    //Packet struct
    //uint32 PingValue;
private:
	virtual void ProcessPacket();

    uint32 m_pingValue;
};

inline uint32 Ping::GetPingValue() {
    return m_pingValue;
}

inline void Ping::SetPingValue(uint32 pingValue) {
	m_pingValue = pingValue;
}

}
}
#endif	/* _PING_H_ */

