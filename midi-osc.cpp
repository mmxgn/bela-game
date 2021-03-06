#include <Bela.h>
#include <Midi.h>
#include <OSCClient.h>
#include <OSCServer.h>
#include <math.h>


// OSC variables
OSCServer oscServer;
OSCClient oscClient;
int localPort = 7562;
int remotePort = 7563;
const char* remoteIp = "127.0.0.1";


std::string encodeNoteOnOff(float midiNote,float velocity, float isOn) {
	std::string sMessage = "";
	sMessage += std::to_string(midiNote);
	sMessage += ',';
	sMessage += std::to_string(velocity);
	sMessage += ',';
	sMessage += std::to_string(isOn);
	return sMessage;
}

std::string encodeControlChange(float data1, float data2) {
	std::string sMessage = "";
	sMessage += std::to_string(data1);
	sMessage += ',';
	sMessage += std::to_string(data2);
	return sMessage;
}
void initOSC() {
#ifdef OSCWS
	//oscReceiver.setup(localPort, on_receive);
	oscSender.setup(remotePort, remoteIp);

	// the following code sends an OSC message to address /osc-setup
	// then waits 1 second for a reply on /osc-setup-reply
	oscSender.newMessage("/osc-setup").send();
#else
	oscServer.setup(localPort);
	oscClient.setup(remotePort, remoteIp);

	// the following code sends an OSC message to address /osc-setup
	// then waits 1 second for a reply on /osc-setup-reply

	bool handshakeReceived = false;
	oscClient.sendMessageNow(oscClient.newMessage.to("/osc-setup").end());
	oscServer.receiveMessageNow(1000);
	while (oscServer.messageWaiting()){
		if (oscServer.popMessage().match("/osc-setup-reply")){
			handshakeReceived = true;
		}
	}

	if (handshakeReceived){
		rt_printf("handshake received!\n");
	} else {
		rt_printf("timeout waiting for OSC handshake!\n");
	}

#endif
}

void sendPositionalDataToBrowser(float x, float y, float a)
{
	oscClient.queueMessage(oscClient.newMessage.to("/osc-player1-xya").add(encodeNoteOnOff(x, y, a)).end());
}

void sendBlockSizeToBrowser(unsigned int index, char value)
{
	const int numBlocks = 8;
	static float blockValues[numBlocks];
	float fValue = value / 127.f;
	float old = blockValues[index];
	float threshold = 0.05;
	if(fabsf(old - fValue) < threshold)
	{
		return;
	}
	blockValues[index] = fValue;
	oscClient.queueMessage(oscClient.newMessage.to("/osc-block-size").add(encodeControlChange(index, fValue)).end());
}

void handleControlChange(char controller, char value)
{
	switch (controller){
		case 18:
		case 22:
		case 26:
		case 30:
		case 48:
		case 52:
		case 56:
		case 60:
			break;
		case 19:
			sendBlockSizeToBrowser(0, value);
			break;
		case 23:
			sendBlockSizeToBrowser(1, value);
			break;
		case 27:
			sendBlockSizeToBrowser(2, value);
			break;
		case 31:
			sendBlockSizeToBrowser(3, value);
			break;
		case 49:
			sendBlockSizeToBrowser(4, value);
			break;
		case 53:
			sendBlockSizeToBrowser(5, value);
			break;
		case 57:
			sendBlockSizeToBrowser(6, value);
			break;
		case 61:
			sendBlockSizeToBrowser(7, value);
			break;
		default:
			break;
	}
}

void midiMessageCallback(MidiChannelMessage message, void* arg){
	if(arg != NULL){
		rt_printf("Message from midi port %s ", (const char*) arg);
	}
	message.prettyPrint();
	int data1 = message.getDataByte(0);
	int data2 = message.getDataByte(1);
	if(message.getType() == kmmControlChange){
#ifdef OSCWS
		oscSender.newMessage("/osc-player2-control").add(encodeControlChange(data1, data2));
		ws_server->send("player2-control-direct", "test1");
#else
		oscClient.queueMessage(oscClient.newMessage.to("/osc-player2-control").add(encodeControlChange(data1, data2)).end());
#endif
	}
	if(message.getType() == kmmNoteOn || message.getType() == kmmNoteOff){
#ifdef OSCWS
		oscSender.newMessage("/osc-player2-note").add(encodeNoteOnOff(data1, data2, message.getType() == kmmNoteOn)).send();
		// ws_server->send("player2-note-direct", "test1");
#else
		oscClient.queueMessage(oscClient.newMessage.to("/osc-player2-note").add(encodeNoteOnOff(data1, data2, message.getType() == kmmNoteOn)).end());
#endif
	}
}


