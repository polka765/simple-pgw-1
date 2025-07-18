#include "data_plane.h"
#include <iostream>

data_plane::data_plane(control_plane &control_plane)
    : _control_plane(control_plane) {}

void data_plane::handle_uplink(uint32_t dp_teid, Packet&& packet) {
    if (auto bearer = _control_plane.find_bearer_by_dp_teid(dp_teid)) {
        if (auto pdn = bearer->get_pdn_connection()) {
            std::cout << "Handling UPLINK traffic:\n"
                      << "  TEID: " << dp_teid << "\n"
                      << "  UE IP: " << pdn->get_ue_ip_addr() << "\n"
                      << "  Packet size: " << packet.size() << " bytes\n"
                      << "  Forwarding to APN: " << pdn->get_apn_gw() << "\n";

            forward_packet_to_apn(pdn->get_apn_gw(), std::move(packet));
        }
    } else {
        std::cout << "ERROR: Bearer not found for UPLINK TEID: " << dp_teid << "\n";
    }
}

void data_plane::handle_downlink(const boost::asio::ip::address_v4& ue_ip,Packet&& packet) {
    if (auto pdn = _control_plane.find_pdn_by_ip_address(ue_ip)) {
        if (auto bearer = pdn->get_default_bearer()) {
            std::cout << "Handling DOWNLINK traffic:\n"
                      << "  UE IP: " << ue_ip << "\n"
                      << "  Packet size: " << packet.size() << " bytes\n"
                      << "  Forwarding to SGW: " << pdn->get_sgw_address() << "\n"
                      << "  Using bearer TEID: " << bearer->get_sgw_dp_teid() << "\n";

            forward_packet_to_sgw(
                pdn->get_sgw_address(),
                bearer->get_sgw_dp_teid(),
                std::move(packet));
        } else {
            std::cout << "ERROR: Default bearer not found for UE IP: " << ue_ip << "\n";
        }
    } else {
        std::cout << "ERROR: PDN connection not found for UE IP: " << ue_ip << "\n";
    }
}