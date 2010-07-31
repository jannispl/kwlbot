addEventHandler("onUserChannelMessage",
	function (user, channel, message)
	{
		var parts = message.split(" ");
		if (parts[0] == "!!")
		{
			var code = message.substr(parts[0].length + 1);
			try
			{
				var res	= eval(code).toString().replace("\r", "").split("\n");

				for (var i = 0; i < res.length; ++i)
				{
					channel.sendMessage(res[i]);
				}
			}
			catch (error)
			{
				channel.sendMessage("Error: " + error.message.replace("\r", "").replace("\n", " "));
			}
			
			return;
		}
	}
);
