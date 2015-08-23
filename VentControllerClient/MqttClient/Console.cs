using System;
using System.Diagnostics;

namespace MqttClient
{
	public static class Console
	{
		private static ErrorConsole _error = null;

		public static void WriteLine(string format, params object[] args)
		{
			Debug.WriteLine (format, args);
		}

		public static ErrorConsole Error 
		{
			get 
			{
				if (_error == null) 
				{
					_error = new ErrorConsole ();
				}

				return _error;
			}
		}
	}

	public class ErrorConsole
	{
		public void WriteLine(string format, params object[] args)
		{
			Debug.WriteLine (format, args);
		}
	}
}

