using System;
using System.Net.Sockets;

namespace MqttClient.Ios
{
	public class IosTcpClient : ITcpClient
	{
		private TcpClient _client;
		private INetworkStream _stream;

		public IosTcpClient ()
		{
			_client = new TcpClient ();
		}

		#region ITcpClient implementation
		public void Connect (string host, int port)
		{
			_client.Connect (host, port);
		}
		public INetworkStream GetStream ()
		{
			if (_stream == null) 
			{
				_stream = new IosNetworkStream (_client.GetStream ());
			}	

			return _stream;
		}
		#endregion
	}
}

