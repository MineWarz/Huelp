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
dpp::snowflake channel;

int main()
{
	/* Create bot cluster */
	dpp::cluster bot(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content);

	/* Output simple log messages to stdout */
	bot.on_log(dpp::utility::cout_logger());

	/* Register slash command here in on_ready */
	bot.on_ready([&bot](const dpp::ready_t& event) {
		if (dpp::run_once<struct clear_bot_commands>()) {
			/* Now, we're going to wipe our commands */
			bot.global_bulk_command_delete();
			/* This one requires a guild id, otherwise it won't know what guild's commands it needs to wipe! */
			bot.guild_bulk_command_delete(800501701999853618);
		}

		/* Wrap command registration in run_once to make sure it doesnt run on every full reconnection */
		if (dpp::run_once<struct register_bot_commands>()) {

			dpp::slashcommand setChannelCommand;
			setChannelCommand.set_name("set-channel")
				.set_description("Set the channel that the bot listens to.")
				.set_application_id(bot.me.id)
				.add_option(
					dpp::command_option(dpp::co_channel, "channel", "The channel that the bot listens to.", true)
				);

			dpp::slashcommand setNumberCommand;
			setNumberCommand.set_name("set-number")
				.set_description("Set a number to start the count with.")
				.set_application_id(bot.me.id)
				.add_option(
					dpp::command_option(dpp::co_integer, "number", "The number that the count starts with.", true)
				);

			vector<dpp::slashcommand> commands {
				{ "ping", "Are you even online?", bot.me.id },
				setChannelCommand,
				setNumberCommand
			};

			bot.global_bulk_command_create(commands);
		}
	});

	/* Handle slash command with the most recent addition to D++ features, coroutines! */
	bot.on_slashcommand([](const dpp::slashcommand_t& event) -> dpp::task<void> {
		if (event.command.get_command_name() == "ping") {
			co_await event.co_reply("Pong!");
		}
		else if (event.command.get_command_name() == "set-channel") {
			channel = get<dpp::snowflake>(event.get_parameter("channel"));
			co_await event.co_reply("Channel set.");
		}
		else if (event.command.get_command_name() == "set-number") {
			currentNumber = (int) get<int64_t>(event.get_parameter("number"));
			valid = true;
			co_await event.co_reply("Number set.");
		}
		co_return;
	});

	bot.on_message_create([&bot](const dpp::message_create_t& event) {
		if (channel == NULL || event.msg.channel_id != channel) {
			return;
		}
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
				handleMistake(bot, event, "Number out of range for integer!");
				valid = false;
			}
		}
		else {
			handleMistake(bot, event, "Message must contain a number!");
		}
	}
}

static void handleNumber(dpp::cluster &bot, const dpp::message_create_t& event, int foundNumber) {
	cout << "match: " << foundNumber << endl;
	if (!valid) {
		if (foundNumber == 0) {
			valid = true;
		} else {
			handleMistake(bot, event, "Next message must be 0!");
		}
	}
	else {
		if (!isValidNextNumber(foundNumber)) {
			handleMistake(bot, event, "Number must be 1 more or less than previous number!");
		}
	}

	if (valid) {
		currentNumber = foundNumber;
		string reaction;

		if (foundNumber == 69) {
			bot.message_add_reaction(event.msg.id, event.msg.channel_id, dpp::unicode_emoji::regional_indicator_n);
			bot.message_add_reaction(event.msg.id, event.msg.channel_id, dpp::unicode_emoji::regional_indicator_i);
			bot.message_add_reaction(event.msg.id, event.msg.channel_id, dpp::unicode_emoji::regional_indicator_c);
			bot.message_add_reaction(event.msg.id, event.msg.channel_id, dpp::unicode_emoji::regional_indicator_e);
			return;
		}

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

static void handleMistake(dpp::cluster& bot, dpp::message_create_t const& event, string userMessage)
{
	bot.message_add_reaction(event.msg.id, event.msg.channel_id, dpp::unicode_emoji::cross_mark);
	event.reply(dpp::message(userMessage));
	valid = false;
	currentNumber = 0;
}

static bool isValidNextNumber(int newNumber) {
	return (newNumber == currentNumber + 1 || newNumber == currentNumber - 1);
}
