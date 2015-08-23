using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace MqttClient.Core.Messages
{
    internal class MqttPingRespMessage : MqttMessage
    {

        public MqttPingRespMessage() : base(MessageType.PINGRESP, 0)
        {
            // Nothing to construct
        }

		public MqttPingRespMessage(INetworkStream str, byte header) : base(str, header)
        {
            // Nothing to construct
        }

		protected override void SendPayload(Stream str)
        {
            // Nothing to send
        }

		protected override void ConstructFromStream(INetworkStream str)
        {
            // Nothing to construct
        }
    }
}
