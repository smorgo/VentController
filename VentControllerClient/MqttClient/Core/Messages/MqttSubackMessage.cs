using System;
using System.Collections.Generic;
using System.Text;

namespace MqttClient.Core.Messages
{
    internal class MqttSubackMessage : MqttAcknowledgeMessage
    {

        private QoS[] grantedQos;

		public MqttSubackMessage(INetworkStream str, byte header) : base(str, header)
        {
          // Nothing to construct
        }

		protected override void ConstructFromStream(INetworkStream str)
        {
            // Read the message ID that the server is acknowlodging
            _ackID = ReadUshortFromStream(str);
            int qosCount = variableHeaderLength - 2;
            grantedQos = new QoS[qosCount];

            for (int i = 0; i < qosCount; i++)
            {
                int res = str.ReadByte();
                if (res != -1)
                {
                    grantedQos[i] = (QoS)res;
                }
                else
                {
                    throw new Exception("Failed to read byte from stream");
                }
            }


        }
    }
}
