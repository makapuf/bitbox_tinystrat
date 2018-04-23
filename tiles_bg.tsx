<?xml version="1.0" encoding="UTF-8"?>
<tileset name="tiles_bg" tilewidth="16" tileheight="16" tilecount="217" columns="7">
 <image source="tiles_bg.png" width="112" height="496"/>
 <terraintypes>
  <terrain name="mountains" tile="-1">
   <properties>
    <property name="defense" type="int" value="4"/>
    <property name="move_boat" type="int" value="99"/>
    <property name="move_wheels" type="int" value="99"/>
   </properties>
  </terrain>
  <terrain name="forest" tile="-1">
   <properties>
    <property name="defense" type="int" value="2"/>
    <property name="move_boat" type="int" value="99"/>
    <property name="move_foot" type="int" value="2"/>
    <property name="move_wheels" type="int" value="3"/>
   </properties>
  </terrain>
  <terrain name="town" tile="-1">
   <properties>
    <property name="defense" type="int" value="3"/>
    <property name="move_boat" type="int" value="99"/>
    <property name="move_foot" type="int" value="2"/>
   </properties>
  </terrain>
  <terrain name="fields" tile="-1">
   <properties>
    <property name="defense" type="int" value="1"/>
    <property name="move_boat" type="int" value="99"/>
   </properties>
  </terrain>
  <terrain name="stable" tile="-1">
   <properties>
    <property name="defense" type="int" value="3"/>
    <property name="move_boat" type="int" value="99"/>
    <property name="move_wheels" type="int" value="99"/>
   </properties>
  </terrain>
  <terrain name="sea" tile="-1">
   <properties>
    <property name="defense" type="int" value="0"/>
    <property name="move_foot" type="int" value="99"/>
    <property name="move_guard2" type="int" value="99"/>
    <property name="move_horse" type="int" value="99"/>
    <property name="move_wheels" type="int" value="99"/>
   </properties>
  </terrain>
  <terrain name="beach" tile="-1">
   <properties>
    <property name="defense" type="int" value="0"/>
   </properties>
  </terrain>
  <terrain name="castle" tile="-1">
   <properties>
    <property name="defense" type="int" value="3"/>
    <property name="move_boat" type="int" value="99"/>
   </properties>
  </terrain>
  <terrain name="camp" tile="-1">
   <properties>
    <property name="defense" type="int" value="2"/>
   </properties>
  </terrain>
  <terrain name="road" tile="0">
   <properties>
    <property name="defense" type="int" value="0"/>
    <property name="move_boat" type="int" value="99"/>
   </properties>
  </terrain>
  <terrain name="plain" tile="0">
   <properties>
    <property name="defense" type="int" value="1"/>
    <property name="move_boat" type="int" value="99"/>
   </properties>
  </terrain>
  <terrain name="river" tile="0">
   <properties>
    <property name="defense" type="int" value="0"/>
    <property name="move_boat" type="int" value="5"/>
   </properties>
  </terrain>
 </terraintypes>
 <tile id="0" terrain="10,10,10,10"/>
 <tile id="1" terrain="10,10,10,10">
  <animation>
   <frame tileid="1" duration="500"/>
   <frame tileid="2" duration="500"/>
   <frame tileid="3" duration="500"/>
   <frame tileid="2" duration="500"/>
  </animation>
 </tile>
 <tile id="2" terrain="10,10,10,10"/>
 <tile id="3" terrain="10,10,10,10"/>
 <tile id="4" terrain="1,1,1,1"/>
 <tile id="5" terrain="1,1,1,1"/>
 <tile id="6" terrain="1,1,1,1"/>
 <tile id="7" terrain="2,2,2,2"/>
 <tile id="8" terrain="2,2,2,2"/>
 <tile id="9" terrain="2,2,2,2"/>
 <tile id="10" terrain="0,0,0,0"/>
 <tile id="11" terrain="0,0,0,0"/>
 <tile id="12" terrain="0,0,0,0"/>
 <tile id="13" terrain="10,10,10,10"/>
 <tile id="14" terrain="2,2,2,2"/>
 <tile id="15" terrain="2,2,2,2"/>
 <tile id="16" terrain="2,2,2,2"/>
 <tile id="17" terrain="2,2,2,2"/>
 <tile id="18" terrain="11,11,11,11">
  <animation>
   <frame tileid="18" duration="250"/>
   <frame tileid="19" duration="250"/>
   <frame tileid="20" duration="250"/>
  </animation>
 </tile>
 <tile id="19" terrain="11,11,11,11"/>
 <tile id="20" terrain="11,11,11,11"/>
 <tile id="21" terrain="2,2,2,2"/>
 <tile id="22" terrain="7,7,7,7"/>
 <tile id="23" terrain="7,7,7,7"/>
 <tile id="24" terrain="7,7,7,7"/>
 <tile id="25" terrain="0,0,0,0"/>
 <tile id="26" terrain="0,0,0,0"/>
 <tile id="29" terrain="7,7,7,7"/>
 <tile id="30" terrain="7,7,7,7"/>
 <tile id="31" terrain="7,7,7,7"/>
 <tile id="32" terrain="0,0,0,0"/>
 <tile id="34" type="mark" terrain="7,7,7,7"/>
 <tile id="37" terrain="8,8,8,8"/>
 <tile id="38" terrain="8,8,8,8"/>
 <tile id="39" terrain="8,8,8,8"/>
 <tile id="40" terrain="8,8,8,8"/>
 <tile id="48" terrain="3,3,3,3"/>
 <tile id="49" terrain="7,7,7,7"/>
 <tile id="50" terrain="7,7,7,7"/>
 <tile id="51" terrain="7,7,7,7"/>
 <tile id="52" terrain="7,7,7,7"/>
 <tile id="53" terrain="7,7,7,7"/>
 <tile id="54" terrain="7,7,7,7"/>
 <tile id="55" terrain="3,3,3,3"/>
 <tile id="56" terrain="7,7,7,7"/>
 <tile id="57" terrain="7,7,7,7"/>
 <tile id="58" terrain="7,7,7,7"/>
 <tile id="59" terrain="7,7,7,7"/>
 <tile id="60" terrain="7,7,7,7"/>
 <tile id="61" terrain="7,7,7,7"/>
 <tile id="62" terrain="3,3,3,3"/>
 <tile id="63" terrain="9,9,9,9"/>
 <tile id="64" terrain="9,9,9,9"/>
 <tile id="65" terrain="9,9,9,9"/>
 <tile id="66" terrain="9,9,9,9"/>
 <tile id="67" terrain="9,9,9,9"/>
 <tile id="68" terrain="9,9,9,9"/>
 <tile id="69" terrain="3,3,3,3"/>
 <tile id="70" terrain="9,9,9,9"/>
 <tile id="71" terrain="9,9,9,9"/>
 <tile id="72" terrain="9,9,9,9"/>
 <tile id="73" terrain="9,9,9,9"/>
 <tile id="74" terrain="9,9,9,9"/>
 <tile id="75" terrain="9,9,9,9"/>
 <tile id="76" terrain="9,9,9,9"/>
 <tile id="77" terrain="9,9,9,9"/>
 <tile id="78" terrain="9,9,9,9"/>
 <tile id="80" terrain="9,9,9,9"/>
 <tile id="81" terrain="9,9,9,9"/>
 <tile id="83" terrain="9,9,9,9"/>
 <tile id="84" terrain="9,9,9,9"/>
 <tile id="86" terrain="9,9,9,9"/>
 <tile id="87" terrain="9,9,9,9"/>
 <tile id="89" terrain="9,9,9,9"/>
 <tile id="90" terrain="4,4,4,4"/>
 <tile id="91" terrain="11,11,11,11"/>
 <tile id="92" terrain="11,11,11,11"/>
 <tile id="93" terrain="11,11,11,11"/>
 <tile id="94" terrain="11,11,11,11"/>
 <tile id="95" terrain="11,11,11,11"/>
 <tile id="98" terrain="11,11,11,11">
  <animation>
   <frame tileid="98" duration="500"/>
   <frame tileid="100" duration="500"/>
  </animation>
 </tile>
 <tile id="99" terrain="11,11,11,11"/>
 <tile id="100" terrain="11,11,11,11"/>
 <tile id="101" terrain="11,11,11,11"/>
 <tile id="102" terrain="11,11,11,11"/>
 <tile id="105" terrain="11,11,11,11"/>
 <tile id="106" terrain="11,11,11,11"/>
 <tile id="107" terrain="11,11,11,11"/>
 <tile id="108" terrain="9,9,9,9"/>
 <tile id="109" terrain="9,9,9,9"/>
 <tile id="110" terrain="9,9,9,9"/>
 <tile id="111" terrain="9,9,9,9"/>
 <tile id="112" terrain="5,5,5,5">
  <animation>
   <frame tileid="112" duration="250"/>
   <frame tileid="113" duration="250"/>
   <frame tileid="114" duration="250"/>
   <frame tileid="115" duration="250"/>
  </animation>
 </tile>
 <tile id="113" terrain="5,5,5,5"/>
 <tile id="114" terrain="5,5,5,5"/>
 <tile id="115" terrain="5,5,5,5"/>
 <tile id="119" terrain="6,6,6,6"/>
 <tile id="120" terrain="6,6,6,6"/>
 <tile id="121" terrain="6,6,6,6"/>
 <tile id="122" terrain="6,6,6,6"/>
 <tile id="123" terrain="6,6,6,6"/>
 <tile id="124" terrain="11,11,11,11"/>
 <tile id="125" terrain="11,11,11,11"/>
 <tile id="126" terrain="6,6,6,6"/>
 <tile id="128" terrain="6,6,6,6"/>
 <tile id="129" terrain="6,6,6,6"/>
 <tile id="130" terrain="6,6,6,6"/>
 <tile id="132" terrain="7,7,7,7"/>
 <tile id="133" terrain="6,6,6,6"/>
 <tile id="134" terrain="6,6,6,6"/>
 <tile id="135" terrain="6,6,6,6"/>
 <tile id="139" terrain="7,7,7,7"/>
 <tile id="140" terrain="7,7,7,7"/>
 <tile id="141" terrain="7,7,7,7"/>
 <tile id="142" terrain="7,7,7,7"/>
 <tile id="143" terrain="7,7,7,7"/>
 <tile id="144" terrain="7,7,7,7"/>
 <tile id="145" terrain="7,7,7,7"/>
 <tile id="146" terrain="7,7,7,7"/>
 <tile id="147" terrain="7,7,7,7"/>
 <tile id="148" terrain="7,7,7,7"/>
 <tile id="149" terrain="7,7,7,7"/>
 <tile id="150" terrain="7,7,7,7"/>
 <tile id="151" terrain="7,7,7,7"/>
 <tile id="152" terrain="7,7,7,7"/>
 <tile id="153" terrain="7,7,7,7"/>
 <tile id="156" terrain="7,7,7,7"/>
 <tile id="157" terrain="7,7,7,7"/>
 <tile id="158" terrain="7,7,7,7"/>
 <tile id="159" terrain="7,7,7,7"/>
 <tile id="161" type="wood"/>
 <tile id="168" type="zero"/>
 <tile id="196" type="P1"/>
 <tile id="203" type="gold"/>
 <tile id="204" type="iron"/>
 <tile id="205" type="wood"/>
 <tile id="206" type="food"/>
</tileset>
