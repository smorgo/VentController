using System;
using System.IO;
using System.Text;

namespace MqttClient.Core.Messages
{
    internal class MqttPubrecMessage : MqttMessage
    {
        private ushort _ackID;

        public ushort AckID
        {
            get
            {
                return _ackID;
            }
        }

        public MqttPubrecMessage(ushort ackID) : base(MessageType.PUBREC, 2)
        {
            _ackID = ackID;
            // Ensure that this message will be re-sent unless acknowledged
            msgQos = QoS.AtLeastOnce;
        }

		public MqttPubrecMessage(INetworkStream str, byte header) : base(str, header)
        {
            // Ensure that this message can get resent in the event of failure
            this.msgQos = QoS.AtLeastOnce;
        }

		protected override void ConstructFromStream(INetworkStream str)
        {
            _ackID = ReadUshortFromStream(str);
        }

		protected override void SendPayload(Stream str)
        {
            WriteToStream(str, _ackID);
        }
    }
}
