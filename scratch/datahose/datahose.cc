/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "include/Core.h"
#include <chrono>

using namespace ns3;

int main (int argc, char *argv[])
{
    if (argc != 2) {
        std::cout << "Missing config TOML file. Usage: <datahose> <config.toml>\"";
        return 1;
    }
    std::string config_file = argv[1];

    std::cout << "Building simulation for config - " << config_file << std::endl;
    setenv("NS_LOG", "Core=all", true);
    LogComponentEnable("Core", LOG_LEVEL_ALL);

    std::unique_ptr<Core> core = std::make_unique<Core>(config_file);

    auto start = std::chrono::steady_clock::now();
    core->runSimulation();
    auto end = std::chrono::steady_clock::now();
    std::cout << "It took " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds" << std::endl;
    return 0;
}