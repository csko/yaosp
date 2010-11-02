#include <ygui++/protocol.hpp>

AppCreate::AppCreate(ipc_port_id replyPort, ipc_port_id clientPort, int flags)
    : m_replyPort(replyPort), m_clientPort(clientPort), m_flags(flags) {}
