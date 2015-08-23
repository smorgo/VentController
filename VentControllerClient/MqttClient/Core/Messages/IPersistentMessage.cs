using System;
namespace MqttClient.Core.Messages
{
    public interface IPersistentMessage
    {
        ushort MessageID { get; }
        MessageType MsgType { get; }
        QoS QualityOfService { get; }
        bool Retained { get; }
        void Serialise(INetworkStream str);
    }
}
