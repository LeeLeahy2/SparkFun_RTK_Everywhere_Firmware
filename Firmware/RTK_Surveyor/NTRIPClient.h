#if !defined(COMPILE_WIFI) && !defined(COMPILE_ETHERNET)

// Remove all of NTRIPClient.h

#else

// Define a hybrid class which can support both WiFiClient and EthernetClient
// Build the Class according to WiFi and Ethernet compile guards

#if defined(COMPILE_WIFI) && defined(COMPILE_ETHERNET)

class NTRIPClient : public WiFiClient, public EthernetClient

#elif defined(COMPILE_ETHERNET)

class NTRIPClient : public EthernetClient

#elif defined(COMPILE_WIFI)

class NTRIPClient : public WiFiClient

#endif

{
  public:
    NTRIPClient(bool useWiFiNotEthernet)
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (HAS_ETHERNET && !useWiFiNotEthernet)
        {
            _ntripClientEthernet = new EthernetClient;
            _useWiFiNotEthernet = false;
        }
        else
        {
            _ntripClientWiFi = new WiFiClient;
            _useWiFiNotEthernet = true;
        }
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
        {
            _ntripClientEthernet = new EthernetClient;
        }
#elif defined(COMPILE_WIFI)
        {
            _ntripClientWiFi = new WiFiClient;
        }
#endif
    };

    ~NTRIPClient()
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
        {
            _ntripClientEthernet->stop();
            delete _ntripClientEthernet;
            _ntripClientEthernet = nullptr;
        }
        else
        {
            // EthernetClient does not have a destructor. It is virtual.
            delete _ntripClientWiFi;
            _ntripClientWiFi = nullptr;
        }
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
        {
            _ntripClientEthernet->stop();
            delete _ntripClientEthernet;
            _ntripClientEthernet = nullptr;
        }
#elif defined(COMPILE_WIFI)
        // EthernetClient does not have a destructor. It is virtual.
        delete _ntripClientWiFi;
        _ntripClientWiFi = nullptr;
#endif
    };

    operator bool()
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet;
        else
            return _ntripClientWiFi;
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet;
        else
            return false;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi;
#endif
    };

    int connect(const char *host, uint16_t port)
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->connect(host, port);
        else
            return _ntripClientWiFi->connect(host, port);
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->connect(host, port);
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->connect(host, port);
#endif
    };

    size_t write(uint8_t b)
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->write(b);
        else
            return _ntripClientWiFi->write(b);
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->write(b);
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->write(b);
#endif
    };

    size_t write(const uint8_t *buf, size_t size)
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->write(buf, size);
        else
            return _ntripClientWiFi->write(buf, size);
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->write(buf, size);
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->write(buf, size);
#endif
    };

    int available()
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->available();
        else
            return _ntripClientWiFi->available();
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->available();
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->available();
#endif
    };

    int read(uint8_t *buf, size_t size)
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->read();
        else
            return _ntripClientWiFi->read();
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->read();
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->read();
#endif
    };

    int read()
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->read();
        else
            return _ntripClientWiFi->read();
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->read();
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->read();
#endif
    };

    void stop()
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            _ntripClientEthernet->stop();
        else
            _ntripClientWiFi->stop();
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            _ntripClientEthernet->stop();
        else
            return;
#elif defined(COMPILE_WIFI)
        _ntripClientWiFi->stop();
#endif
    };

    uint8_t connected()
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->connected();
        else
            return _ntripClientWiFi->connected();
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->connected();
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->connected();
#endif
    };

    size_t print(const char *printMe)
    {
#if defined(COMPILE_ETHERNET) && defined(COMPILE_WIFI)
        if (!_useWiFiNotEthernet)
            return _ntripClientEthernet->print(printMe);
        else
            return _ntripClientWiFi->print(printMe);
#elif defined(COMPILE_ETHERNET)
        if (HAS_ETHERNET)
            return _ntripClientEthernet->print(printMe);
        else
            return 0;
#elif defined(COMPILE_WIFI)
        return _ntripClientWiFi->print(printMe);
#endif
    };

  protected:
    bool _useWiFiNotEthernet;
#if defined(COMPILE_WIFI)
    WiFiClient *_ntripClientWiFi;
#endif
#if defined(COMPILE_ETHERNET)
    EthernetClient *_ntripClientEthernet;
#endif
};

#endif