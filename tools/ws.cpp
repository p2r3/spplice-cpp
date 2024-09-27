#include <iostream>
#include <QtWebSockets/QWebSocket>
#include "../globals.h" // Project globals

// Definitions for this source file
#include "ws.h"

WebSocketClient::WebSocketClient (const QUrl &url, QObject *parent) : QObject(parent), m_url(url) {

  connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
  connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onMessage);

  m_webSocket.open(m_url);

}

void WebSocketClient::onConnected () {
  std::cout << "WebSocket connected!\n";
  m_webSocket.sendTextMessage(QStringLiteral("Hello, WebSocket!"));
}

void WebSocketClient::onMessage (const QString &message) {
  std::cout << "Message received: " << message.toStdString() << '\n';
}
