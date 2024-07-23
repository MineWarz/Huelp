#include "Huelp.h"
#include <dpp/dpp.h>
#include <dpp/unicode_emoji.h>
#include <regex>

using namespace std;

/* Be sure to place your token in the line below.
 * Follow steps here to get a token:
 * https://dpp.dev/creating-a-bot-application.html
 * When you invite the bot, be sure to invite it with the 
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */
const string    BOT_TOKEN    = "SAMPLE TOKEN";
int currentNumber = 0;
bool valid = false;

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
			vector<dpp::slashcommand> commands {
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
		checkCount(bot, event);
	});

	/* Start the bot */
	bot.start(dpp::st_wait);

	return 0;
}

static void checkCount(dpp::cluster& bot, dpp::message_create_t const& event) {
	if (!event.msg.author.is_bot()) {
		string const msgContent = event.msg.content;
		cout << "Found message: " << msgContent << endl;

		regex numberFormat("-?\\d+");
		smatch match;

		if (regex_search(msgContent.begin(), msgContent.end(), match, numberFormat)) {
			try {
				int foundNumber = stoi(match[0]);
				handleNumber(bot, event, foundNumber);
			}
			catch (out_of_range const &e)
			{
				cout << e.what() << endl;
				handleMistake(event, "Number out of range for integer!");
				valid = false;
			}
		}
		else {
			handleMistake(event, "Message must contain a number!");
		}
	}
}

static void handleNumber(dpp::cluster &bot, const dpp::message_create_t& event, int foundNumber) {
	cout << "match: " << foundNumber << endl;
	if (!valid) {
		if (foundNumber == 0) {
			valid = true;
		} else {
			handleMistake(event, "Next message must be 0!");
		}
	}
	else {
		if (!isValidNextNumber(foundNumber)) {
			handleMistake(event, "Number must be 1 more or less than previous number!");
		}
	}

	if (valid) {
		currentNumber = foundNumber;
		string reaction;
		switch (foundNumber) {
			case 0: reaction = dpp::unicode_emoji::glowing_star;
				break;
			case 10: reaction = dpp::unicode_emoji::keycap_ten;
				break;
			case 100: reaction = dpp::unicode_emoji::_100;
				break;
			case 321: reaction = dpp::unicode_emoji::rocket;
				break;
			default: reaction = dpp::unicode_emoji::white_check_mark;
				break;
		}
		bot.message_add_reaction(event.msg.id, event.msg.channel_id, reaction);
	}
}

static void handleMistake(dpp::message_create_t const& event, string userMessage)
{
	event.reply(dpp::message(userMessage));
	valid = false;
	currentNumber = 0;
}

static bool isValidNextNumber(int newNumber) {
	return (newNumber == currentNumber + 1 || newNumber == currentNumber - 1);
}
