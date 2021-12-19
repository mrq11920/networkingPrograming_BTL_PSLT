/*
* @author tunc2112
* @group INT3304 1 - PSLT
* Chương trình hiển thị thông tin cảm biến A đóng vai trò là Subscriber (bên đăng ký nhận thông tin)
*/

import java.net.*;
import java.io.*;
import java.util.*;
import java.util.regex.*;
import java.util.Scanner;
import java.nio.charset.StandardCharsets;

public class Subscriber {
	private static final int BUFFER_LENGTH = 4096;

	private static final String COMMAND_QUIT = "QUIT";

	private static Pattern PATTERN_TOPIC = Pattern.compile(
		"^\\{\"topic\":\"([^\"]+)\",\"datetime\":\"([^\"]+)\",\"temperature\":\"([^\"]+)\",\"humidity\":\"([^\"]+%)\"\\}"
	);	

	private Socket socket           = null;
	private DataInputStream  input  = null;
	private DataOutputStream output = null;

	public Subscriber(String address, int port) {
		try {
			System.out.println("[+]Subscriber Socket Created!");
			socket = new Socket(address, port);
			System.out.println("[+]Connected to Broker\n" + getHelp());

			input  = new DataInputStream(socket.getInputStream());
			output = new DataOutputStream(socket.getOutputStream());
		}
		catch (UnknownHostException e) {
			System.out.println(e);
		}
		catch (IOException e) {
			System.out.println(e);
		}

		Scanner stdin = new Scanner(System.in);
		while (true) {
			try {
				System.out.print("Subscriber: ");
				System.out.flush();

				String line = stdin.nextLine();
				System.out.println("Input: " + line);
				output.write(line.getBytes());
				output.flush();

				String response = "";
				byte[] response_bytes = new byte[BUFFER_LENGTH];
				while (true) {
					int read_bytes;
					try {
						read_bytes = input.read(response_bytes);
						if (read_bytes > 0) {
							response += new String(response_bytes, 0, read_bytes, StandardCharsets.UTF_8);
						}
						System.out.println(response);
						if (read_bytes < response_bytes.length) {
							break;
						}
					}
					catch (IOException e) {
						System.out.println(e);
						break;
					}
				}

				if (response.startsWith("{\"available_topics\":") && response.endsWith("\"}")) {
					System.out.println("Broker: " + getAvailableTopics(response));
				} else if (response.startsWith("{\"topic\":") && response.endsWith("\"}")) {
					System.out.println("Broker: " + getTopicInfo(response));
				} else {
					System.out.println("Broker: " + response);
				}
				if (line.equals(COMMAND_QUIT)) {
					break;
				}
			}
			catch (IOException e) {
				System.out.println(e);
			}
		}

		// close the connection
		try {
			input.close();
			output.close();
			socket.close();
		}
		catch (IOException e) {
			System.out.println(e);
		}
	}

	public static String getTopicInfo(String data) {
		// {"topic":"locationA/sensorA","datetime":"Sun Nov 28 17:50:51","temperature":"35","humidity":"56%"}
		Matcher matcher = PATTERN_TOPIC.matcher(data);
		String result = "";
		if (matcher.matches()) {
			String topic_field = matcher.group(1);
			String datetime_field = matcher.group(2);
			String temperature_field = matcher.group(3);
			String humidity_field = matcher.group(4);
			result = String.format(
				"At %s, about the topic %s:\n- Temperature: %s\n- Humidity: %s",
				datetime_field, topic_field, temperature_field, humidity_field
			);
		}
		return result;
	}

	public static String getAvailableTopics(String data) {
		if (!data.startsWith("{\"available_topics\":") || !data.endsWith("\"}")) {
			return "";
		}
		String topic_string = data.substring(21, data.length() - 2);
		if (topic_string.length() == 0) {
			return "There are no available topics!";
		}
		String[] topics = topic_string.split(",");
		String result = "Available topics:";
		for (String topic: topics) {
			result += "\n- " + topic;
		}
		return result;
	}

	public static String getHelp() {
		return
			"Note: “QUIT” to quit\n" + 
			"Input “TOPIC” to subscribe an available topic\n" + 
			"Input “SUB” to view all available topic then input a topic to subscribe to it\n" + 
			"Input “LIST” to view all subscribed topics";
	}

	public static void main(String[] args) {
		String ip_address = args.length > 0 ? args[0] : "127.0.0.1";
		Subscriber Subscriber = new Subscriber(ip_address, 9111);
	}
}