using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace MqttClient.Core.Messages
{
    internal class MqttDisconnectMessage : MqttMessage
    {
        public MqttDisconnectMessage() : base( MessageType.DISCONNECT, 0 )
        {
          // Nothing to construct
        }

		protected override void SendPayload(Stream str)
        {
            // No payload to send
        }

		protected override void ConstructFromStream(INetworkStream str)
        {
            throw new Exception("Protocol does not support receiving DISCONNECT message");
        }
    }
}
