addEventHandler("onUserChannelMessage",
	function (bot, user, channel, message)
	{
		var parts = message.split(" ");

		if (parts[0] == "!!")
		{
			var code = message.substr(parts[0].length + 1);
			try
			{
				var res	= eval(code);
			}
			catch (error)
			{
				bot.sendMessage(channel, "Error: " + error.message);
			}
		}
	}
);

TestFunc();