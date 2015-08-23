using System;
using System.Threading;
using System.Diagnostics.Contracts;
using System.Threading.Tasks;

namespace MqttClient
{
	internal delegate void TimerCallback(object state);

	internal sealed class Timer : CancellationTokenSource, IDisposable
	{
		internal Timer(TimerCallback callback, object state, int dueTime, int period)
		{
//			var options = (TaskContinuationOptions)(TaskContinuationOptions.ExecuteSynchronously | TaskContinuationOptions.OnlyOnRanToCompletion);

//			Contract.Assert(period == -1, "This stub implementation only supports dueTime.");

			Task.Run (() => Run (callback, state, dueTime, period));
/*
 * Task.Delay(dueTime, Token).ContinueWith((t, s) =>
				{
					var tuple = (Tuple<TimerCallback, object>)s;
					tuple.Item1(tuple.Item2);
				}, 
				Tuple.Create(callback, state), 
				CancellationToken.None,
				options,
				TaskScheduler.Default);
*/
		}

		private void Run(TimerCallback callback, object state, int dueTime, int period)
		{
			var isPeriodic = (period != -1);

			while (true) 
			{
				Task.Delay (dueTime, Token).Wait ();
				callback(state);

				if (!isPeriodic) 
				{
					break;
				}

				dueTime = period;
			}
		}

		public void Change (int kmillis, int kmillis2)
		{
			throw new NotImplementedException ();
		}

		public new void Dispose() { Cancel(); }
	}
}

