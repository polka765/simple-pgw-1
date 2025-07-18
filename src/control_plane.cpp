#include "control_plane.h"
#include <stdexcept>
#include <vector>
#include <boost/asio/ip/address.hpp>
#include <iostream>

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_cp_teid(uint32_t cp_teid) const {
    auto it = _pdns.find(cp_teid);

    return it != _pdns.end() ? it->second : nullptr;
}

std::shared_ptr<pdn_connection> control_plane::find_pdn_by_ip_address(const boost::asio::ip::address_v4& ip) const {
    auto it = _pdns_by_ue_ip_addr.find(ip);

    return it != _pdns_by_ue_ip_addr.end() ? it->second : nullptr;
}

std::shared_ptr<bearer> control_plane::find_bearer_by_dp_teid(uint32_t dp_teid) const {
    auto it = _bearers.find(dp_teid);

    return it != _bearers.end() ? it->second : nullptr;
}

std::shared_ptr<pdn_connection> control_plane::create_pdn_connection(const std::string& apn, boost::asio::ip::address_v4 sgw_addr, uint32_t sgw_cp_teid) {
    auto apn_it = _apns.find(apn);
    if (apn_it == _apns.end()) {
        std::cout << "ERROR: APN '" << apn << "' not found\n";
        throw std::runtime_error("APN not found");
    }

    static uint32_t next_ip = 0xC0A80101;
    boost::asio::ip::address_v4 ue_ip(next_ip++);

    auto pdn = pdn_connection::create(sgw_cp_teid, apn_it->second, ue_ip);
    pdn->set_sgw_addr(sgw_addr);
    pdn->set_sgw_cp_teid(sgw_cp_teid);

    auto default_bearer = create_bearer(pdn, sgw_cp_teid);
    pdn->set_default_bearer(default_bearer);

    _pdns[sgw_cp_teid] = pdn;
    _pdns_by_ue_ip_addr[ue_ip] = pdn;

    std::cout << "Created new PDN connection:\n"
              << "  APN: " << apn << "\n"
              << "  UE IP: " << ue_ip << "\n"
              << "  SGW address: " << sgw_addr << "\n"
              << "  Control TEID: " << sgw_cp_teid << "\n";

    return pdn;
}

void control_plane::delete_pdn_connection(uint32_t cp_teid) {
    if (auto it = _pdns.find(cp_teid); it != _pdns.end()) {
        auto pdn = it->second;
        _pdns_by_ue_ip_addr.erase(pdn->get_ue_ip_addr());

        std::cout << "Deleting PDN connection:\n"
                  << "  UE IP: " << pdn->get_ue_ip_addr() << "\n"
                  << "  Bearers count: " << pdn->_bearers.size() << "\n";

        std::vector<uint32_t> teids;
        for (const auto& [teid, _] : pdn->_bearers) {
            teids.push_back(teid);
        }
        for (auto teid : teids) {
            delete_bearer(teid);
        }
        _pdns.erase(it);
        std::cout << "PDN connection deleted successfully\n";
    } else {
        std::cout << "WARNING: PDN connection with TEID " << cp_teid << " not found for deletion\n";
    }
}

std::shared_ptr<bearer> control_plane::create_bearer(const std::shared_ptr<pdn_connection>& pdn, uint32_t sgw_teid) {
    static uint32_t next_dp_teid = 1;
    auto new_bearer = std::make_shared<bearer>(next_dp_teid++, *pdn);
    new_bearer->set_sgw_dp_teid(sgw_teid);

    _bearers[new_bearer->get_dp_teid()] = new_bearer;
    pdn->add_bearer(new_bearer);

    std::cout << "Created new bearer:\n"
              << "  PGW TEID: " << new_bearer->get_dp_teid() << "\n"
              << "  SGW TEID: " << sgw_teid << "\n"
              << "  UE IP: " << pdn->get_ue_ip_addr() << "\n";

    return new_bearer;
}

void control_plane::delete_bearer(uint32_t dp_teid) {
    if (auto it = _bearers.find(dp_teid); it != _bearers.end()) {
        auto bearer = it->second;
        if (auto pdn = bearer->get_pdn_connection()) {
            pdn->remove_bearer(dp_teid);
        }

        _bearers.erase(it);
        std::cout << "Deleted bearer with TEID: " << dp_teid << "\n";
    } else {
        std::cout << "WARNING: Bearer with TEID " << dp_teid << " not found for deletion\n";
    }
}

void control_plane::add_apn(std::string apn_name, boost::asio::ip::address_v4 apn_gateway) {
    _apns[std::move(apn_name)] = apn_gateway;

    std::cout << "Added new APN: " << apn_name
              << " with gateway: " << apn_gateway << "\n";
}