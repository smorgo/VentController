using System;
using System.Net.Sockets;
using System.IO;

namespace MqttClient.Ios
{
	public class IosNetworkStream : INetworkStream
	{
		private NetworkStream _stream;

		public IosNetworkStream (NetworkStream stream)
		{
			_stream = stream;
		}

		#region implemented abstract members of NetworkStream

		public int ReadByte ()
		{
			return _stream.ReadByte ();
		}

		public void Write (byte[] buffer, int offset, int count)
		{
			_stream.Write (buffer, offset, count);
		}

		public void WriteByte (byte data)
		{
			_stream.WriteByte (data);
		}

		public IAsyncResult BeginRead (byte[] buffer, int offset, int count, AsyncCallback callback, object state)
		{
			return _stream.BeginRead (buffer, offset, count, callback, state);
		}

		public void Close ()
		{
			_stream.Close ();
		}

		public int EndRead (IAsyncResult asyncResult)
		{
			return _stream.EndRead (asyncResult);
		}

		public Stream Stream {get { return _stream; }}

		#endregion
	}
}

