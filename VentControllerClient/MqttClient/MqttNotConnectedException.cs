using System;
using System.Text;

namespace MqttClient
{
    public class MqttNotConnectedException : MqttException
    {
        public MqttNotConnectedException() : base() { }

        public MqttNotConnectedException(string message) : base(message) { }

        public MqttNotConnectedException(string message, Exception innerException) : base(message, innerException) { }

        
    }
}
