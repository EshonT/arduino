#include <dht11.h>    //引用DHT11头文件
#include <SPI.h>      //引用SPI头文件
#include <Ethernet.h> //引用W5100头文件

#define DHT_PIN A0
#define POST_INTERVAL (5 * 1000)

String TEMP_UNIT = "&deg;C";

// 设定MAC地址、IP地址
// IP地址需要参考你的本地网络设置
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // mac地址
IPAddress ip(192, 168, 0, 233);                    // IP地址

//----------------------------------
// 初始化Ethernet库
// HTTP默认端口为80
EthernetServer server(80); //设置Arduino的网页服务器
dht11 DHT11;               //设置温湿度模块的结构参数

// 设置参数的程序区段，只会执行一次
void setup()
{
    Serial.begin(9600); //启动串行通信来观察Arduino运行情况
    // 开始ethernet连接，并作为服务器初始化
    Ethernet.begin(mac, ip);       //启动网络功能，设置MAC和IP地址
    server.begin();                //启动网页服务器功能
    Serial.print("Server is at "); //显示Arduino自己的IP
    Serial.println(Ethernet.localIP());
}

// 程序会重复执行
void loop()
{
    DHT11.read(DHT_PIN);
    short TEMP = DHT11.temperature;
    short HUM = DHT11.humidity;

    EthernetClient client = server.available();
    if (client)
    {
        Serial.println("new client");
        //一个HTTP的连接请求，以空行结尾
        boolean currentLineIsBlank = true;
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                Serial.write(c);

                // 如果收到空白行，说明http请求结束，并发送响应消息

                if (c == '\n' && currentLineIsBlank)
                {
                    // 标准的HTTP响应标头信息
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: text/html");
                    client.println("Connection: close"); // 在反应后将关闭连接
                    client.println("Refresh: 5");        // 每5秒更新一次网页
                    client.println();                    //响应标头的后面需要一个空行
                    client.println("");                  //类型定义，说明此为HTML信息
                    client.println("");
                    client.println("T&H"); //网页的标题
                    //网页内容信息
                    client.print("<br>");
                    client.println((float)TEMP, 2);
                    client.print(TEMP_UNIT + "<br>");
                    client.println((float)HUM, 2);
                    client.print("%RH<br>");

                    client.print("<br>Dew Point ("+ TEMP_UNIT +") : <br>");
                    client.println(dewPoint(TEMP, HUM));
                    client.print("\t[Standard]<br>");
                    client.println(dewPointFast(TEMP, HUM));
                    client.print("[Fast]");

                    client.println("");
                    break; //跳出while循环，避免浏览器持续处于接收状态
                }
                if (c == '\n')
                    // 已经开始一个新行
                    currentLineIsBlank = true;
                else if (c != '\r')
                    currentLineIsBlank = false;
            }
        }
        delay(2000);                           //停留一些时间让浏览器接收Arduino传送的数据
        client.stop();                         //关闭连接
        Serial.println("client disconnected"); //串口打印client断开连接。
    }
}

/**
 * @brief 摄氏温度度转化为华氏温度
 *
 * @param celsius 摄氏度
 * @return double 华氏度
 */
double Fahrenheit(double celsius)
{
    return 1.8 * celsius + 32;
}

/**
 * @brief 摄氏温度转化为开氏温度
 *
 * @param celsius 摄氏度
 * @return double 开尔文
 */
double Kelvin(double celsius)
{
    return celsius + 273.15;
}

/**
 * @brief 露点（点在此温度时，空气饱和并产生露珠）
 * 参考: http://wahiduddin.net/calc/density_algorithms.htm
 *
 * @param celsius 温度
 * @param humidity 湿度
 * @return double
 */
double dewPoint(double celsius, double humidity)
{
    double A0 = 373.15 / (273.15 + celsius);
    double SUM = -7.90298 * (A0 - 1);
    SUM += 5.02808 * log10(A0);
    SUM += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / A0))) - 1);
    SUM += 8.1328e-3 * (pow(10, (-3.49149 * (A0 - 1))) - 1);
    SUM += log10(1013.246);
    double VP = pow(10, SUM - 3) * humidity;
    double T = log(VP / 0.61078); // temp var
    return (241.88 * T) / (17.558 - T);
}

/**
 * @brief 露点的快速计算方式，速度是5倍dewPoint()
 * 参考: http://en.wikipedia.org/wiki/Dew_point
 *
 * @param celsius
 * @param humidity
 * @return double
 */
double dewPointFast(double celsius, double humidity)
{
    double a = 17.271;
    double b = 237.7;
    double temp = (a * celsius) / (b + celsius) + log(humidity / 100);
    double Td = (b * temp) / (a - temp);
    return Td;
}
