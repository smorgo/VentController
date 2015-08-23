using System;
using System.IO;
using System.Text;

namespace MqttClient.Core.Messages
{
    internal class MqttPubcompMessage : MqttMessage
    {
        private ushort _ackID;

        public ushort AckID
        {
            get
            {
                return _ackID;
            }
        }

        public MqttPubcompMessage(ushort ackID) : base(MessageType.PUBCOMP, 2)
        {
            _ackID = ackID;
        }

		public MqttPubcompMessage(INetworkStream str, byte header): base(str, header)
        {
            // Nothing to construct
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
