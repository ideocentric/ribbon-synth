int sw1 = 32;
int sw2 = 31;

void setup() {
  // put your setup code here, to run once:
  pinMode(sw1, INPUT);
  pinMode(sw2, INPUT);
  Serial.begin(9600);
}

String pad4(String str)
{
  int len;
  String out;
  out = "";
  len = str.length();
  while(len<4){
    out = " " + out;
  }
  return out + str;
}

void loop() {
  // put your main code here, to run repeatedly:
  int sw1val, sw2val;
  // top left black xoo
  Serial.print("\tA09:  ");
  Serial.print(analogRead(A9));
  // top center white oxo
  Serial.print("\tA00: ");
  Serial.print(analogRead(A0));
  // top right gray oox
  Serial.print("\A08:  ");
  Serial.print(analogRead(A8));
  // switchest
  sw1val = digitalRead(sw1);
  sw2val = digitalRead(sw2);
  Serial.print("sw1: ");
  Serial.print(sw1val); 
  Serial.print("\tsw2: "); 
  Serial.print(sw2val);

  // bottom left 1 orange xoooo
  Serial.print("\tA03: ");
  Serial.print(analogRead(A3));
  // botom left 2 yellow oxooo
  Serial.print("\tA6: ");
  Serial.print(analogRead(A6));
  // bottom center 3 green ooxoo
  Serial.print("\tA12: ");
  Serial.print(analogRead(A12));
  // bottom right 4 blue oooxo
  Serial.print("\tA17: ");
  Serial.print(analogRead(A17));
  // bottom right purple oooox
  Serial.print("\tA1:  ");
  Serial.print(analogRead(A1));

  // force sensor
  Serial.print("\tA14: ");
  Serial.print(analogRead(A14));
  // soft pot
  Serial.print("\tA15: ");
  Serial.print(analogRead(A15));



  Serial.println();
  delay(100);
}
