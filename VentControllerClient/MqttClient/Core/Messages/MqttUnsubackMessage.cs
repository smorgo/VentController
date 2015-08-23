using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace MqttClient.Core.Messages
{
    internal class MqttUnsubackMessage : MqttAcknowledgeMessage
    {

		public MqttUnsubackMessage(INetworkStream str, byte header) : base(str, header)
        {
          // Nothing to construct
        }

		protected override void ConstructFromStream(INetworkStream str)
        {
            _ackID = ReadUshortFromStream(str);
        }
    }
}
