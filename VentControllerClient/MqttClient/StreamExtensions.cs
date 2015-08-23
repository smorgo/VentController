using System;
using System.IO;

namespace MqttClient
{
	public static class DateTimeExtensions
	{
		public static string ToShortDateString(this DateTime dateTime)
		{
			return dateTime.ToString("d");
		}

		public static string ToLongTimeString(this DateTime dateTime)
		{
			return dateTime.ToString("T");
		}

	}

	public static class StreamWriterExtensions
	{
		public static void Close(this StreamWriter writer)
		{
			writer.Dispose ();
		}
	}
}

