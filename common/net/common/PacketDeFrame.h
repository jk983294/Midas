#ifndef MIDAS_PACKET_DEFRAME_H
#define MIDAS_PACKET_DEFRAME_H

#include "MidasHeader.h"
#include "midas/MidasConstants.h"
#include "midas/MidasException.h"
#include "net/buffer/ConstBufferSequence.h"

namespace midas {

/**
 * object used to split up contents of a packet into its component messages in a zero copy manner.
 * The template parameter is a class which implements the deFramePacket function
 * that creates a list of frames ( ConstBuffers ) based off the inPacket.
 * The total number of bytes actually processed into the frames is returned from the function.
 */
template <class MessageType>
class PacketDeFrame {
private:
    PacketDeFrame() = delete;

public:
    ~PacketDeFrame() {}

    static size_t deframe_packet(const ConstBuffer& inPacket, ConstBufferSequence& messageList, size_t& nNumMessages) {
        return MessageType::deframe_packet(inPacket, messageList, nNumMessages);
    }
};

class MidasMessage {
public:
    MidasMessage();
    ~MidasMessage();

    /**
     * Implementation required for the PacketDeFrame object.
     * This function uses the Midas header to determine individual messages and will separate them out
     * into separate const buffers if required. If not required, the function will find the full number
     * of whole messages and send back as a single buffer
     *  @param inPacket The inbound packet containing the original packet data.
     *  @param messageList The list of const_buffers containing the pointer/sizes of each message.
     *         Any partial message at the end of the packet will not be included.
     *  @param nNumMessages Returns the number of individual messages found.
     *  @return The number of bytes from the inPacket that was processed from the inPacket.
     */
    static size_t deframe_packet(const ConstBuffer& inPacket, ConstBufferSequence& messageList, size_t& nNumMessages) {
        register const char* pPositionPtr{inPacket.data()};
        register size_t nBytesRemaining{inPacket.size()};
        register size_t nOffset{0};
        nNumMessages = 0;

        messageList.clear();

        while (nBytesRemaining >= sizeof(MidasHeader)) {
            const MidasHeader* header = (const MidasHeader*)pPositionPtr;
            if (header->flag == 0xFF) {
                uint32_t messageSize = ntohs(header->len) + 2;
                if (nBytesRemaining >= messageSize) {
                    messageList.push_back(ConstBuffer(inPacket, nOffset, messageSize));
                    ++nNumMessages;

                    nBytesRemaining -= messageSize;
                    pPositionPtr += messageSize;
                    nOffset += messageSize;
                } else {
                    // Not enough data left to complete the message, so finish up.
                    break;
                }
            } else {
                THROW_MIDAS_EXCEPTION("Invalid Midas packet. Possible data corruption or invalid data format");
            }
        }

        size_t nBytesUsed = inPacket.size() - nBytesRemaining;
        return nBytesUsed;
    }

    static void split_header_payload(const ConstBuffer& message, ConstBuffer& header, ConstBuffer& payload) {
        size_t headerSize = midas_header_size();

        header = ConstBuffer(message.data(), headerSize);
        payload = ConstBuffer(message.data() + headerSize, message.size() - headerSize);
    }
};

typedef PacketDeFrame<MidasMessage> MidasMessageDeFrame;
}

#endif
