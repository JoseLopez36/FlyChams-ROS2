#pragma once

// Standard includes
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <fstream>
#include <iostream>
#define _USE_MATH_DEFINES
#include <cmath>
#include <memory>

/* XLSX Files Management: https://github.com/troldal/OpenXLSX */
#define NOMINMAX
#include <OpenXLSX.hpp>

// Core includes
#include "flychams_core/types/core_types.hpp"
#include "flychams_core/types/config_types.hpp"
#include "flychams_core/utils/math_utils.hpp"

namespace flychams::core
{
	/**
	 * ════════════════════════════════════════════════════════════════
	 * @brief Spreadsheet parser for mission configuration
	 * ════════════════════════════════════════════════════════════════
	 * @author Jose Francisco Lopez Ruiz
	 * @date 2025-01-25
	 * ════════════════════════════════════════════════════════════════
	 */
	class SpreadsheetParser
	{
	public: // Types
		using XLDocument = OpenXLSX::XLDocument;
		using XLWorkbook = OpenXLSX::XLWorkbook;
		using XLWorksheet = OpenXLSX::XLWorksheet;
		using XLRow = OpenXLSX::XLRow;
		using XLCell = OpenXLSX::XLCell;

	public: // Public methods
		/**
		 * @brief Parse mission configuration from spreadsheet
		 * @param path Path to input spreadsheet file
		 * @return Mission configuration pointer
		 */
		static core::MissionConfigPtr parseSpreadsheet(const std::string& path);

	private: // Implementation methods
		static void parseMission(OpenXLSX::XLWorkbook& book, core::MissionConfigPtr config_ptr);
		static void parseEnvironment(OpenXLSX::XLWorkbook& book, core::MissionConfigPtr config_ptr);
		static void parseTargetGroup(OpenXLSX::XLWorkbook& book, core::MissionConfigPtr config_ptr);
		static void parseAgentTeam(OpenXLSX::XLWorkbook& book, core::MissionConfigPtr config_ptr);
		static void parseTracking(OpenXLSX::XLWorkbook& book, core::AgentConfigPtr agent_ptr);
		static void parseMultiCameraSet(OpenXLSX::XLWorkbook& book, core::AgentConfigPtr agent_ptr);
		static void parseMultiWindowSet(OpenXLSX::XLWorkbook& book, core::AgentConfigPtr agent_ptr);
		static void parseDroneModel(OpenXLSX::XLWorkbook& book, core::AgentConfigPtr agent_ptr);
		static void parseCameraModel(OpenXLSX::XLWorkbook& book, core::MultiCameraConfigPtr multi_camera_ptr);
		static void parseGimbalModel(OpenXLSX::XLWorkbook& book, core::MultiCameraConfigPtr multi_camera_ptr);

	private: // Helper methods
		// Check if a cell is empty
		static bool isCellEmpty(const OpenXLSX::XLCell& cell)
		{
			return cell.empty() || cell.value().type() == OpenXLSX::XLValueType::Empty;
		}

		// Get the value of a cell or fail
		template <typename T>
		static T getCellValue(const OpenXLSX::XLCell& cell)
		{
			try
			{
				const OpenXLSX::XLValueType cell_type = cell.value().type();
				const std::string cell_ref = cell.cellReference().address();

				// Handle empty cells
				if (isCellEmpty(cell))
				{
					throw std::runtime_error("Empty cell at " + cell_ref + ".");
				}

				// Handle - as empty value (does not care about this)
				if (cell_type == OpenXLSX::XLValueType::String && cell.value().get<std::string>() == "-")
				{
					return T();
				}

				// Direct type match
				if constexpr (std::is_same_v<T, std::string>)
				{
					if (cell_type == OpenXLSX::XLValueType::String)
					{
						std::string val = cell.value().get<std::string>();
						return val;
					}
				}
				else if constexpr (std::is_same_v<T, bool>)
				{
					if (cell_type == OpenXLSX::XLValueType::Boolean)
					{
						bool val = cell.value().get<bool>();
						return val;
					}
				}
				else if constexpr (std::is_arithmetic_v<T>)
				{
					if (cell_type == OpenXLSX::XLValueType::Integer)
					{
						int val = cell.value().get<int>();
						return val;
					}
					else if (cell_type == OpenXLSX::XLValueType::Float)
					{
						float val = cell.value().get<float>();
						return val;
					}
				}
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("Error loading cell: " + std::string(e.what()));
			}

			return T();
		}

		// Parse a string to a vector of a given type
		template <typename T>
		static std::vector<T> parseVector(const std::string& input, size_t expected_count, const char separator)
		{
			std::vector<T> result;

			// Handle empty input
			if (input.empty())
			{
				return result;
			}

			// Sanitize input: remove all non-numeric/non-delimiter characters
			std::string clean_input = input;
			auto is_valid_char = [separator](char c)
				{
					return std::isdigit(c) || c == '-' || c == '.' || c == separator;
				};

			clean_input.erase(
				std::remove_if(clean_input.begin(), clean_input.end(),
					[&](char c)
					{ return !is_valid_char(c); }),
				clean_input.end());

			std::istringstream iss(clean_input);
			std::string token;
			size_t index = 0;

			try
			{
				while (std::getline(iss, token, separator))
				{
					// Remove leading/trailing whitespace from each token
					token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());

					if (token.empty())
						continue;

					std::istringstream converter(token);
					T value;
					if (!(converter >> value))
					{
						throw std::runtime_error("Failed to convert '" + token + "' to type");
					}

					result.push_back(value);
					index++;
				}

				if (expected_count > 0 && result.size() != expected_count)
				{
					std::cout << "Expected " << expected_count << " elements, got " << result.size() << "." << std::endl;
					throw std::invalid_argument("Expected " + std::to_string(expected_count) +
						" elements, got " + std::to_string(result.size()));
				}
			}
			catch (const std::exception& e)
			{
				std::cout << "Failed to parse: " << e.what() << "." << std::endl;
				throw;
			}

			return result;
		}
	};

} // namespace flychams::core