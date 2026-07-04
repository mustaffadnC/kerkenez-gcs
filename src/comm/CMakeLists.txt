add_library(kerkenez_comm STATIC
    ILink.h
    LinkConfig.h
    LinkManager.h
    LinkManager.cpp
    SerialLink.h
    SerialLink.cpp
    TcpLink.h
    TcpLink.cpp
    UdpLink.h
    UdpLink.cpp
)

target_link_libraries(kerkenez_comm
    PUBLIC kerkenez_core Qt6::Network Qt6::SerialPort
)
