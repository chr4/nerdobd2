#!/usr/bin/ruby

require 'socket'

require 'rubygems'
require 'json'

require 'dm-core'
require 'dm-migrations'
require 'dm-validations'
require 'dm-timestamps'

require 'sinatra'


file = "/tmp/nerdobd2_socket"

DataMapper::Logger.new($stdout, :debug)
#DataMapper.setup(:default, 'sqlite:///tmp/nerdobd.sqlite3')
DataMapper.setup(:default, 'sqlite::memory:')
#DataMapper.setup(:default, 'postgres://postgres@localhost/nerdobd2')


class EngineData
  include DataMapper::Resource

  property :id,                 Serial
  property :created_at,         DateTime
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
  property :created_at,         DateTime
  property :temp_engine,        Float
  property :temp_air_intake,    Float
  property :voltage,            Float
end

DataMapper.auto_upgrade!


get '/' do
  return "Hello World"
end

# save engine data to database
post '/engine_data' do
  EngineData.create(params)
end

# save other data to database
post '/other_data' do
  OtherData.create(params)
end

