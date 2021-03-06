% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/Parameters.R
\name{Parameters.new}
\alias{Parameters.new}
\title{Creates a new OpenABM Parameters object (wrapper for
\code{\link{Parameters}$new()})}
\usage{
Parameters.new(
  input_param_file = NA_character_,
  param_line_number = 1,
  output_file_dir = "./",
  input_households = NA_character_,
  hospital_input_param_file = NA_character_,
  hospital_param_line_number = NA_integer_,
  read_param_file = TRUE,
  read_hospital_param_file = FALSE
)
}
\arguments{
\item{input_param_file}{Input parameters CSV file path.
Optional, default:
\code{system.file("default_params", "baseline_parameters.csv", package = "OpenABMCovid19")}}

\item{param_line_number}{Which column of the input param file to read.}

\item{output_file_dir}{Where to write output files to.}

\item{input_households}{Household demographics file.
Optional, default:
\code{system.file("default_params", "baseline_household_demographics.csv", package = "OpenABMCovid19")}}

\item{hospital_input_param_file}{Hospital input parameters CSV file path.
Optional, default:
\code{system.file("default_params", "hospital_baseline_parameters.csv", package = "OpenABMCovid19")}}

\item{hospital_param_line_number}{Which column of the hospital input
param file to read.}

\item{read_param_file}{A boolean. If \code{TRUE}, read
\code{input_param_file}. If \code{FALSE}, ignore
\code{input_param_file}.}

\item{read_hospital_param_file}{A boolean. If \code{TRUE}, read
\code{hospital_input_param_file}. If \code{FALSE}, ignore
\code{hospital_input_param_file}.}
}
\description{
Creates a new OpenABM Parameters object (wrapper for
\code{\link{Parameters}$new()})
}
\seealso{
\href{https://github.com/BDI-pathogens/OpenABM-Covid19/blob/master/documentation/parameters/parameter_dictionary.md}{Online Documentation}.
}
