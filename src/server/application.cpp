#include "application.h"

#include <SFML/System/Clock.hpp>
#include <common/launch_config.h>
#include <common/network/commands.h>
#include <iostream>
#include <thread>

namespace server {
    Application::Application(const LaunchConfig &config, port_t port)
        : m_server(config.serverOptions.maxConnections, port, m_entities)
    {
        std::cout << "Server started on port " << port << "." << std::endl;

        for (unsigned i = m_server.maxConnections(); i < m_entities.size();
             i++) {
            m_entities[i].isAlive = true;
        }

        for (int z = 0; z < WORLD_SIZE; ++z) {
            for (int x = 0; x < WORLD_SIZE; ++x) {
                m_chunks.emplace_back(x, z);
            }
        }
    }

    void Application::run(sf::Time timeout)
    {
        sf::Clock timeoutClock;
        sf::Clock deltaClock;
        while (m_isRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            m_server.recievePackets();
            update(deltaClock.restart());
            sendState();

            if (m_server.connectedPlayes() == 0) {
                if (timeoutClock.getElapsedTime() > timeout) {
                    m_isRunning = false;
                }
            }
            else {
                timeoutClock.restart();
            }
        }
    }

    void Application::update(sf::Time deltaTime)
    {
        // update here
        m_server.updatePlayers();

        int id = 0;
        for (auto &entity : m_entities) {
            if (entity.isAlive) {
                entity.position += entity.velocity * deltaTime.asSeconds();
                entity.velocity.x *= 0.8f;
                entity.velocity.z *= 0.8f;
                if (id >= m_server.maxConnections()) {
                    entity.tick();
                }
            }
            id++;
        }

        m_server.resendPackets();
    }

    void Application::sendState()
    {
        auto statePacket = m_server.createPacket(CommandToClient::WorldState,
                                                 Packet::Flag::None);
        auto &payload = statePacket.payload;
        payload << static_cast<u16>(m_entities.size());
        for (entityid_t i = 0; i < m_entities.size(); i++) {
            if (m_entities[i].isAlive) {
                Entity &entity = m_entities[i];

                payload << i;
                payload << entity.position.x << entity.position.y
                        << entity.position.z;
                payload << entity.rotation.x << entity.rotation.y;
            }
        }
        m_server.sendToAllClients(statePacket);
    }
} // namespace server
