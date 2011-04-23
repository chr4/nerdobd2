#!/usr/bin/ruby

require 'socket'

require 'rubygems'
require 'json'

require 'dm-core'
require 'dm-timestamps'

require 'sinatra'


DataMapper::Logger.new($stdout, :debug)
DataMapper.setup(:default, 'sqlite:///dev/shm/nerdob2.sqlite3')


class EngineData
  include DataMapper::Resource

  property :id,                 Serial
  property :time,               DateTime
  property :rpm,                Float
  property :speed,              Float
  property :injection_time,     Float
  property :oil_pressure,       Float
  property :con_per_h,          Float
  property :con_per_100km,      Float
end

class OtherData
  include DataMapper::Resource

  property :id,                 Serial
  property :time,               DateTime
  property :temp_engine,        Float
  property :temp_air_intake,    Float
  property :voltage,            Float
end



get '/' do
  data = EngineData.get(1)
  return "Hello World: #{data.inspect}"
end
