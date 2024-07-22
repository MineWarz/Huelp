#include "MyBot.h"
#include <dpp/dpp.h>
#include <regex>

/* Be sure to place your token in the line below.
 * Follow steps here to get a token:
 * https://dpp.dev/creating-a-bot-application.html
 * When you invite the bot, be sure to invite it with the 
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */
const std::string    BOT_TOKEN    = "SAMPLE TOKEN";

int main()
{
	/* Create bot cluster */
	dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);

	/* Output simple log messages to stdout */
	bot.on_log(dpp::utility::cout_logger());

	/* Register slash command here in on_ready */
	bot.on_ready([&bot](const dpp::ready_t& event) {
		/* Wrap command registration in run_once to make sure it doesnt run on every full reconnection */
		if (dpp::run_once<struct register_bot_commands>()) {
			std::vector<dpp::slashcommand> commands {
				{ "ping", "Are you even online?", bot.me.id }
			};

			bot.global_bulk_command_create(commands);
		}
	});

	/* Handle slash command with the most recent addition to D++ features, coroutines! */
	bot.on_slashcommand([](const dpp::slashcommand_t& event) -> dpp::task<void> {
		if (event.command.get_command_name() == "ping") {
			co_await event.co_reply("Pong!");
		}
		co_return;
	});

	bot.on_message_create([&bot](const dpp::message_create_t& event) {
		checkCount(event);
	});

	/* Start the bot */
	bot.start(dpp::st_wait);

	return 0;
}

static void checkCount(const dpp::message_create_t& event) {
	if (!event.msg.author.is_bot()) {
		std::string const msgContent = event.msg.content;
		std::cout << "Found message: " << msgContent << std::endl;

		std::regex number("\\d+");
		std::smatch match;

		if (std::regex_search(msgContent.begin(), msgContent.end(), match, number)) {
			std::cout << "match: " << match[0] << '\n';
		}
	}
}
