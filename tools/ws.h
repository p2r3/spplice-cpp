#ifndef TOOLS_WS_H
#define TOOLS_WS_H

#include <QObject>
#include <QtWebSockets/QWebSocket>

class WebSocketClient : public QObject {
  Q_OBJECT

  public:
    explicit WebSocketClient (const QUrl &url, QObject *parent = nullptr);

  private slots:
    void onConnected ();
    void onMessage (const QString &message);

  private:
    QWebSocket m_webSocket;
    QUrl m_url;

};

#endif
