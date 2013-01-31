float p_0 = 100.0; // starting 
float p_halflife = 604800.0; // in seconds
float t = 0.0; // counter for time (seconds)
float p_t = 0.0; // p on time t;
float p_decayed = p_0; // instead of calculating directly, accumulate it by decaying

void setup() {
  Serial.begin(9600);
  Serial.print("BEGIN\n");
  delay(200);
  
  float p_1 = 1.0f;
  float p_2 = p_1 * pow(2, -5.0f / p_halflife);
  float p_decay = p_1 - p_2;
  Serial.println(p_2, 16);

  for (unsigned long i = 0; i <= 7 * 86400; i += 5) {
    p_t = p_0 * pow(2, -t / p_halflife);
    
    if (i % 3600 == 0) {
      Serial.print("time: ");
      Serial.print(t, 0);
      Serial.print(" p_t: ");
      Serial.print(p_t, 9);
      Serial.print(" p_decayed: ");
      Serial.print(p_decayed, 9);
      Serial.print(" diff: ");
      Serial.println(p_t - p_decayed, 9);
    }
    
    t += 5.0;
    p_decayed *= p_2;
  }
  Serial.print("Used parameter: ");
  Serial.println(p_2, 16);
}

void loop()
{

  delay(5000);
}
