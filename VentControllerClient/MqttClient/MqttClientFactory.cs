using System;
using System.Text;
using MqttClient.Core;
using MqttClient;

namespace MqttClient
{
    public class MqttClientFactory
    {
        public static IMqtt CreateClient(string connString, string clientId, string username = null, string password = null, IPersistence persistence = null)
        {
            return new Mqtt(connString, clientId, username, password, persistence);
        }

        public static IMqttShared CreateSharedClient(string connString, string clientId, string username = null, string password = null)
        {
            return new Mqtt(connString, clientId, username, password, null);
        }

        public static IMqtt CreateBufferedClient(string connString, string clientId)
        {
            throw new NotImplementedException();
        }
    }
}
