
CREATE TABLE "Users" (
	"Id_user"	INTEGER,
	"Username"	TEXT,
	"Password"	TEXT,
	"Status"	INTEGER
);
CREATE TABLE "Messages" (
	"Id_message"	INTEGER,
	"Id_transmitter"	INTEGER,
	"Id_receiver"	INTEGER,
	"Text"	TEXT,
	"Id_reply"	INTEGER,
	"Seen"	INTEGER
);
INSERT INTO "Users" VALUES 
(1,"Mara","parola",0);
