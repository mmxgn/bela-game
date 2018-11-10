#include <Bela.h>
#include <Midi.h>
#include <OSCClient.h>
#include <OSCServer.h>


// OSC variables
OSCServer oscServer;
OSCClient oscClient;
int localPort = 7562;
int remotePort = 7563;
const char* remoteIp = "127.0.0.1";


std::string encodeNoteOnOff(int midiNote, int velocity, bool isOn) {
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

void sendDataToBrowser(float pitch, float amp)
{
	oscClient.queueMessage(oscClient.newMessage.to("/osc-player1-xy").add(encodeControlChange(pitch, amp)).end());
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

