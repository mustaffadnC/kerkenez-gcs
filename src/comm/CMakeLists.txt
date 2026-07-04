add_library(kerkenez_comm STATIC
    ILink.h
    TcpLink.h
    TcpLink.cpp
)

target_link_libraries(kerkenez_comm
    PUBLIC kerkenez_core Qt6::Network Qt6::SerialPort
)
