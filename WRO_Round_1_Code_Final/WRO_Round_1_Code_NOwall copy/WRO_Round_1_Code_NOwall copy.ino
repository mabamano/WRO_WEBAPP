// ########### Loop ############################################################################################################ //
// MODIFIED: Turn counting instead of color sensors for lap counting
// Ultrasonic sensors handle navigation, positioning, and turning decisions
int turn_chk_count = 12;  // 3 Laps - 12 turns, 1 Lap - 4 turns.
int turn_count = 0;// 12
//#---Bot Speeds---######################################################################
int normal_speed = 240;//pwm
int turn_speed = 230;//pwm
int turn_delay = 1800;//ms
//#######################################################################################
int fus_slow_speed = 220;//pwm
int fus_slow_dist = 130;//cm
//#---Servo Angles---####################################################################
int servo_center = 100;//deg
int left_turn_angle = servo_center - 25; //80 deg
int right_turn_angle = servo_center + 25;//120 deg
//####################################################################
// Color sensor constants removed - using turn counting instead
//####################################################################
bool lt_st_count = 0;
bool rt_st_count = 0;

bool left_right_arc_turn = 1;
bool left_right_r_turn = 0;
bool turning_in_progress = 0; // Prevent continuous turning
bool stuck_detected = 0; // Track if bot is stuck
unsigned long stuck_start_time = 0; // Time when stuck detection started
int prev_l_us = 0, prev_r_us = 0, prev_f_us = 0; // Previous ultrasonic values for stuck detection
unsigned long turn_timeout = 0; // Track turning timeout
//#########################################################################################//
#define DPDT_Push_Button_Pin 34

// Color sensor variables removed - using turn counting instead
int f_us, f1_us, f2_us, b_us, l_us, r_us, fusa, far;

bool LOGIC_LOCK = 1; //1 True state.
bool DPDT_STATE = 0; //0 False state.
//#########################################################################################//

void loop() {
  
  DPDT_STATE = digitalRead(DPDT_Push_Button_Pin);
  //Serial.println("DPDT Button State : "+String(DPDT_STATE));

  US_Values(f_us, f1_us, f2_us, b_us, l_us, r_us);
  // Serial.println("F_US : " + String(f_us) + " | F1_US : " + String(f1_us) + " | F2_US : " + String(f2_us) + 
  //             " | B_US : " + String(b_us) + 
  //             " | L_US : " + String(l_us) + " | R_US : " + String(r_us));

  // Color sensors removed - using turn counting instead
  
  if (DPDT_STATE == 1) 
  { 
    if (LOGIC_LOCK == 1) 
    { 
      // Check for turning timeout first
      if (is_turning_timed_out())
      {
        return; // Skip normal navigation this cycle
      }
      
      // Check for stuck condition
      if (check_stuck())
      {
        stuck_recovery();
        return; // Skip normal navigation this cycle
      }
      
      // Always run ultrasonic-based navigation
      side_us_logic_fun();
    }     
  }
  else
  {
    bot_shutdown();
  }

  if (turn_count == turn_chk_count) 
  {
    end_stop();
    bot_shutdown();
    LOGIC_LOCK = 0;
    turn_count = 0; 
  }   

}


// Color sensor functions removed - using turn counting instead

// Stuck detection function
bool check_stuck()
{
  int buffer = 5; // 5cm buffer for stuck detection
  unsigned long current_time = millis();
  
  // Check if all ultrasonic values are within buffer range
  bool l_same = abs(l_us - prev_l_us) <= buffer;
  bool r_same = abs(r_us - prev_r_us) <= buffer;
  bool f_same = abs(f_us - prev_f_us) <= buffer;
  
  if (l_same && r_same && f_same && (l_us != 0) && (r_us != 0) && (f_us != 0))
  {
    if (!stuck_detected)
    {
      stuck_detected = 1;
      stuck_start_time = current_time;
      Serial.println("Stuck detection started");
    }
    else if (current_time - stuck_start_time >= 3000) // 3 seconds
    {
      Serial.println("Bot is stuck! Initiating recovery...");
      return true;
    }
  }
  else
  {
    stuck_detected = 0;
    stuck_start_time = 0;
  }
  
  // Update previous values
  prev_l_us = l_us;
  prev_r_us = r_us;
  prev_f_us = f_us;
  
  return false;
}

// Recovery function when stuck
void stuck_recovery()
{
  Serial.println("Executing stuck recovery...");
  motor_stop();
  rgb_led(255, 255, 255); // White LED for recovery
  
  // Determine which side has lower ultrasonic value
  if (l_us < r_us)
  {
    // Left side closer, turn right and reverse
    Serial.println("Turning right and reversing");
    moveServoTo(right_turn_angle);
    delay(1000);
    motor_backward(200);
    delay(1500); // Reverse for 1.5 seconds
  }
  else
  {
    // Right side closer, turn left and reverse
    Serial.println("Turning left and reversing");
    moveServoTo(left_turn_angle);
    delay(1000);
    motor_backward(200);
    delay(1500); // Reverse for 1.5 seconds
  }
  
  // Return to center and continue
  moveServoTo(servo_center);
  delay(500);
  rgb_led(0, 0, 0);
  
  // Reset stuck detection
  stuck_detected = 0;
  stuck_start_time = 0;
}

// Check if turning has timed out
bool is_turning_timed_out()
{
  if (turning_in_progress && (millis() - turn_timeout > 5000)) // 5 second timeout
  {
    Serial.println("Turning timed out, forcing stop");
    turning_in_progress = 0;
    motor_stop();
    moveServoTo(servo_center);
    return true;
  }
  return false;
}

void turn_count_fun()
{
   //# Count turns instead of color lines #/////////////////////////////////////////////////////////////////
   // This function is called when a turn is completed (left or right)
   // Each turn represents 1/4 of a lap, so 12 turns = 3 complete laps
   turn_count++;
   Serial.println("Turn Count : " + String(turn_count) + " / " + String(turn_chk_count));
   rgb_led(0, 150, 0); // Green LED for turn count
   delay(100);
   rgb_led(0, 0, 0);
}

void side_us_logic_fun()
{              
    // Check for no walls detected up to 100cm on either side
    bool left_no_wall = (l_us > 100) || (l_us == 0);
    bool right_no_wall = (r_us > 100) || (r_us == 0);
    
    // Debug output
    // Serial.println("L_US: " + String(l_us) + " | R_US: " + String(r_us) + " | L_NoWall: " + String(left_no_wall) + " | R_NoWall: " + String(right_no_wall));
    
    // If no wall detected on one side, stop and turn (only if not already turning)
    if (left_no_wall && !right_no_wall && !turning_in_progress) {
        // No wall on left, turn left
        turning_in_progress = 1;
        turn_timeout = millis(); // Set timeout start time
        motor_stop();
        rgb_led(255, 0, 0); // Red LED for left turn
        delay(1000); // Stop for 1 second
        
        // Turn left while moving forward
        moveServoTo(left_turn_angle);
        motor_forward(turn_speed);
        
        // Continue turning until wall is detected on left side or timeout
        unsigned long turn_start = millis();
        while ((l_us > 50 || l_us == 0) && (millis() - turn_start < 3000)) {
            US_Values(f_us, f1_us, f2_us, b_us, l_us, r_us);
            delay(50);
        }
        
        moveServoTo(servo_center);
        rgb_led(0, 0, 0);
        turning_in_progress = 0;
        turn_count_fun(); // Count the completed left turn
        return;
    }
    else if (right_no_wall && !left_no_wall && !turning_in_progress) {
        // No wall on right, turn right
        turning_in_progress = 1;
        turn_timeout = millis(); // Set timeout start time
        motor_stop();
        rgb_led(0, 0, 255); // Blue LED for right turn
        delay(1000); // Stop for 1 second
        
        // Turn right while moving forward
        moveServoTo(right_turn_angle);
        motor_forward(turn_speed);
        
        // Continue turning until wall is detected on right side or timeout
        unsigned long turn_start = millis();
        while ((r_us > 50 || r_us == 0) && (millis() - turn_start < 3000)) {
            US_Values(f_us, f1_us, f2_us, b_us, l_us, r_us);
            delay(50);
        }
        
        moveServoTo(servo_center);
        rgb_led(0, 0, 0);
        turning_in_progress = 0;
        turn_count_fun(); // Count the completed right turn
        return;
    }
    
    // Normal forward movement with speed control based on front distance
    if(f_us < fus_slow_dist){motor_forward(fus_slow_speed);}
    else{motor_forward(normal_speed);}

    // Center positioning using left and right ultrasonic sensors - equalize distances
    if ((l_us != 0) && (r_us != 0)) // Both sensors have valid readings
    {
        int distance_diff = l_us - r_us; // Positive = left is farther, Negative = right is farther
        int tolerance = 5; // 5cm tolerance for centering
        
        if (abs(distance_diff) > tolerance) // If difference is significant
        {
            if (distance_diff > 0) // Left side is farther, turn left to center
            {
                rgb_led(0, 0, 0);
                rgb_led(255, 0, 50); // Red LED for left adjustment
                moveServoTo(servo_center - 10); // Turn slightly left to center
                
                if(rt_st_count == 0)
                {
                  if(lt_st_count == 0)
                  {
                    lt_st_count = 1;
                  }
                }
            }
            else // Right side is farther, turn right to center
            {
                rgb_led(0, 0, 0);
                rgb_led(255, 0, 50); // Red LED for right adjustment
                moveServoTo(servo_center + 10); // Turn slightly right to center
                
                if(lt_st_count == 0)
                {
                  if(rt_st_count == 0)
                  {
                    rt_st_count = 1;
                  }
                }
            }
        }
        else // Bot is centered within tolerance
        {
            rgb_led(0, 0, 0);
            rgb_led(255, 255, 0); // Yellow LED for centered
            moveServoTo(servo_center); // Stay centered
        }
        
        // Debug output for centering
        // Serial.println("L_US: " + String(l_us) + " | R_US: " + String(r_us) + " | Diff: " + String(distance_diff));
    }
    else if ((l_us < 30) && (l_us != 0)) // Only left sensor has close wall
    { 
        rgb_led(0, 0, 0);
        rgb_led(255, 0, 50); // Red LED for left adjustment
        moveServoTo(servo_center + 10); // Turn slightly right to center

        if(rt_st_count == 0)
        {
          if(lt_st_count == 0)
          {
            lt_st_count = 1;
          }
        }
    }
    else if ((r_us < 30) && (r_us != 0)) // Only right sensor has close wall
    {
        rgb_led(0, 0, 0);
        rgb_led(255, 0, 50); // Red LED for right adjustment
        moveServoTo(servo_center - 10); // Turn slightly left to center

        if(lt_st_count == 0)
        {
          if(rt_st_count == 0)
          {
            rt_st_count = 1;
          }
        }
    } 
}

void bot_shutdown()
{
  motor_stop();
  moveServoTo(servo_center);
  rgb_led(0, 0, 0);
}

void left_stop()
{
  rgb_led(0, 0, 0);
  delay(1);
  rgb_led(255, 255, 255);
  
  if(left_right_arc_turn)
  {
    motor_forward(210);
    delay(1000);
  }
  moveServoTo(left_turn_angle);
  delay(1500);
  moveServoTo(right_turn_angle);
  delay(1500);
  moveServoTo(servo_center);
  delay(1000);
  
  rgb_led(0, 0, 0);
}

void right_stop()
{
  rgb_led(0, 0, 0);
  delay(1);
  rgb_led(255, 255, 255);

  if(left_right_arc_turn)
  {
    motor_forward(210);
    delay(1000);
  }
  moveServoTo(right_turn_angle);
  delay(1500);
  moveServoTo(left_turn_angle);
  delay(1500);
  moveServoTo(servo_center);
  delay(1000);

  rgb_led(0, 0, 0);

}

void end_stop()
{
  // Stop at the same position as start after 12 turns
  Serial.println("12 turns completed - stopping at start position");
  
  // Determine which side was used more for stopping
  if(lt_st_count == 1)
  {
    left_stop();
  }
  else if(rt_st_count == 1)
  {
    right_stop();
  }
  else
  {
    // Default stop if no specific side was tracked
    motor_stop();
    rgb_led(255, 255, 255); // White LED for completion
    delay(2000);
    rgb_led(0, 0, 0);
  }
}