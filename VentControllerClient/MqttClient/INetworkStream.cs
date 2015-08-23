using System;
using System.IO;

namespace MqttClient
{
	public interface INetworkStream 
	{
		IAsyncResult BeginRead(byte[] buffer, int offset, int count, AsyncCallback callback, Object state);

		void Close();

		int EndRead(IAsyncResult asyncResult);

		int ReadByte ();

		Stream Stream { get; }

		void Write (byte[] buffer, int offset, int count);

		void WriteByte (byte data);
	}
}

