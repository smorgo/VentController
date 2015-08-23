using System;
using System.IO;

namespace MqttClient
{
	public interface ITcpClient
	{
		void Connect (string host, int port);

		INetworkStream GetStream();
	}
}

