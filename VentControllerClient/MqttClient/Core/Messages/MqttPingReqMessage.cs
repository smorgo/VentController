using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace MqttClient.Core.Messages
{
    internal class MqttPingReqMessage : MqttMessage
    {
        public MqttPingReqMessage() : base(MessageType.PINGREQ, 0)
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
